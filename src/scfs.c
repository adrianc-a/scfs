/**
 * Author: Adrian Chmielewski-Anders
 * Date: 2 May 2015
 */

#define FUSE_USE_VERSION 29

#if __STDC_VERSION >= 199901L
#define _XOPEN_SOURCE 600
#else
#define _XOPEN_SOURCE 500
#endif

#include <time.h>
#include <fuse.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h>
#include <errno.h>
#include <sys/stat.h>
#include <syslog.h>

#include "sc.h"

sc_track_t *current_track = NULL;

static sc_track_t *sc_get_song(sc_t *info, const char *path);

/**
 * Go from /user/playlist/song to ["user", "playlist", "song"]
 * return: array_t* of char*
 */
static array_t *sc_split_path(const char *path) {
    const char *delim = "/";
    array_t *ret = arrnew();

    char *dup = strdup(path);
    char *user = strtok(dup, delim);

    if (!user) {
        free(user);
        return ret;
    }

    arradd(ret, (void*)strdup(user));

    while (1) {
        char *next = strtok(NULL, delim);
        if (!next) break;
        arradd(ret, (void*)strdup(next));
    }

    free(dup);

    return ret;
}

static int sc_getattr(const char *path, struct stat* info) {
    int res = 0;

    const array_t *args = sc_split_path(path);

    memset(info, 0, sizeof(struct stat));
    if (!strcmp(path, "/")) {
        info->st_mode = S_IFDIR | 0775;
        info->st_nlink = 2;
    } else if (args->length < 3){
        info->st_mode = S_IFDIR | 0444;
        info->st_nlink = 1;
    } else {
        info->st_mode = S_IFREG | 0440;
        info->st_nlink = 1;
        sc_track_t *tmp = sc_get_song((sc_t*)fuse_get_context()->private_data, path);
        if (!tmp) {
            res = -ENOENT;
        } else {
            info->st_size = tmp->data->size;
            sc_track_free(tmp);
        }
    }

    arrdstryfree(args);
    return res;
}

static int sc_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static sc_track_t *sc_get_song(sc_t *info, const char *path) {
    array_t *elems = sc_split_path(path);
    char *save = info->username;
    info->username = arrget(elems, 0);
    sc_track_t *ret = malloc(sizeof(sc_track_t));
    bool found = false;

    if (elems->length == 3) {
        const array_t *playlists = sc_get_playlists(info);
        for (int i = 0; i < playlists->length; i++) {
            sc_playlist_t *list = arrget(playlists, i);
            if (!strcmp(list->title, arrget(elems, 1))) {
                for (int j = 0; j < list->tracks->length; j++) {
                    sc_track_t *track = arrget(list->tracks, j);
                    if (!strcmp(track->title, arrget(elems, 2))) {
                        sc_track_cpy(ret, track); 
                        found = true;
                        break;
                    }
                }
            }
        }
    }

    info->username = save;
    arrdstryfree(elems);

    if (!found) {
        free(ret);
        return NULL;
    }

    ret->data->data = NULL;
    return ret;
}


/**
 * Read a song into a buffer, can't read a directory, if so, return an error
 */
static int sc_read(const char *path, char *buffer, size_t size, off_t offset, 
        struct fuse_file_info *fi) {
    (void)fi;

    sc_t *info = (sc_t*)fuse_get_context()->private_data;

    if (!current_track) {
        current_track = sc_get_song(info, path);
    }

    if (!current_track->data->data) {
        sc_get_track_stream(info, current_track);
    }

    const buf_t *bytes = current_track->data;
    size_t len = bytes->size;

    if (offset >= (long)len) {
        size = 0;
    }

    if (size > len - offset) {
        size = len - offset;
    }

    memcpy(buffer, bytes->data + offset, size);

    return size;
}

static int sc_release(const char *path, struct fuse_file_info *fi) {
    if (current_track) {
        syslog(LOG_USER | LOG_INFO, "release: %s\n", path);
        sc_track_free(current_track);
        current_track = NULL;
    }
    return 0;
}

static int sc_opendir(const char *path, struct fuse_file_info *fi) {
    return 0;
}

/**
 * If reading / just show the single username of the user. If not, then show the
 * user that is desired.
 */
static int sc_readdir(const char *path, void *buffer, fuse_fill_dir_t filler, 
        off_t offset, struct fuse_file_info *fi) {

    (void)offset;
    (void)fi;

    sc_t *info = (sc_t*)fuse_get_context()->private_data;
    char *save = info->username;

    array_t *args = sc_split_path(path);

    if (args->length >= 1) {
        info->username = arrget(args, 0);
    }

    if (args->length == 0) {
        return -ENOENT;
    } else if (args->length == 1) { // /user  -- display playlists
        const array_t *playlists = sc_get_playlists(info);
        for (int i = 0; i < playlists->length; i++) {
            sc_playlist_t *playlist = (sc_playlist_t*)arrget(playlists, i);
            filler(buffer, playlist->title, NULL, 0);
        }

        arrdstry(playlists, sc_playlist_free_void);
    } else if (args->length == 2) { // /user/playlist -- display songs
        const array_t *playlists = sc_get_playlists(info);
        for (int i = 0; i < playlists->length; i++) {
            sc_playlist_t *list = arrget(playlists, i);
            if (!strcmp(list->title, arrget(args, 1))) {
                for (int j = 0; j < list->tracks->length; j++) {
                    sc_track_t *track = arrget(list->tracks, j);
                    filler(buffer, track->title, NULL, 0);
                }
            }
            break;
        }
        arrdstry(playlists, sc_playlist_free_void);
    }

    // for navigaiton
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    // don't eliminate the original username
    info->username = save;
    return 0;
}

static int sc_releasedir(const char *path, struct fuse_file_info *fi) {
    return 0;
}

// cleanup
static void sc_destroy(void *buffer) {
    sc_t *info = (sc_t*)fuse_get_context()->private_data; 
    sc_free(info);
}

static struct fuse_operations sc_ops = {
    .getattr = sc_getattr,
    .open = sc_open,
    .read = sc_read,
    .release = sc_release,
    .opendir = sc_opendir,
    .readdir = sc_readdir,
    .releasedir = sc_releasedir,
    .destroy = sc_destroy
};

static void sc_usage() {
    printf("usage: scfs [FUSE opts] CLIENT_ID username\n");
}

int main(int argc, char *argv[]) {
    if (getuid() == 0 || geteuid() == 0) {
        fprintf(stderr, "Should not scfs as root\n");
        return 1;
    }

    if (argc < 4) {
        sc_usage();
        return 1;
    }

    openlog("scfs", LOG_NDELAY | LOG_PID, LOG_USER);

    sc_t* sc_info = sc_new(argv[argc - 1], argv[argc - 2]);

    argv[--argc] = NULL;
    argv[--argc] = NULL;
    return fuse_main(argc, argv, &sc_ops, sc_info);
}
