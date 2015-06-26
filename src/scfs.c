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
#include "utils.h"

// map of path to desired action
enum sc_action_e {
    ROOT = 0,
    ME,
    TRACKS,
    TRACK,
    PLAYLISTS,
    PLAYLIST,
    TRACK_FROM_PLAYLIST,
    SEARCH
};

sc_track_t *current_track = NULL;

static sc_track_t *sc_get_song(sc_t *info, const char *path);
static array_t *sc_split_path(const char *path);
static bool sc_is_dir(const char *path);
static enum sc_action_e sc_get_action(array_t *path);
static void sc_populate_root(void *buffer, fuse_fill_dir_t filler);
static void sc_populate_playlists(sc_t *info, void *buffer, 
                                  fuse_fill_dir_t filler);
static void sc_populate_me(void *buffer, fuse_fill_dir_t filler);
static void sc_populate_tracks(sc_t *info, void *buffer, fuse_fill_dir_t filler);
static void sc_populate_playlists(sc_t *info, void *buffer, 
                                  fuse_fill_dir_t filler);
static void sc_populate_playlist(sc_t *info, char *name, void *buffer,
                                 fuse_fill_dir_t filler);
static void sc_populate_search(sc_t *info, char *q, void *buffer, 
                               fuse_fill_dir_t filer);

static bool sc_get_track_from_user(sc_t *info, array_t *elems, sc_track_t *ret);
static bool sc_get_track_from_playlist(sc_t *info, array_t *elems, sc_track_t *ret);

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

/**
 * Directory: 
 *   - /
 *   - /username/tracks
 *   - /username/playlists
 *   - /username/playlists/playlistname
 *   - /me/tracks
 *   - /me/playlists
 *   - /me/playlists/playlistname
 *   - /search/query
 * File:
 *   - not one of above
 * todo: the checks to see if it's a directory or file could be more strict
 */
static bool sc_is_dir(const char *path) {
    array_t *args = sc_split_path(path);

    if ((args->length  < 3) |
           (args->length == 1 && !strcmp(arrget(args, 0), "me")) |
           (args->length == 3 && !strcmp(arrget(args, 0), "search")) |
           (args->length == 2 && !strcmp(arrget(args, 1), "tracks")) |
           (args->length > 1 && args->length < 4 && 
            !strcmp(arrget(args, 1), "playlists"))) {
        return true;
    }

    arrdstryfree(args);
    return false;
}

static void sc_populate_root(void *buffer, fuse_fill_dir_t filler) {
    filler(buffer, "me", NULL, 0);
    filler(buffer, "search", NULL, 0);
}

static void sc_populate_me(void *buffer, fuse_fill_dir_t filler) {
    filler(buffer, "tracks", NULL, 0);
    filler(buffer, "playlists", NULL, 0);
}

static void sc_populate_playlists(sc_t *info, void *buffer, 
                                  fuse_fill_dir_t filler) {

    const array_t *playlists = sc_get_playlists(info);

    for (int i = 0; i < playlists->length; i++) {
        sc_playlist_t *playlist = (sc_playlist_t*)arrget(playlists, i);
 
        // / is replaced by -
        STR_REPLACE_CHAR(playlist->title, '/', '-');
        filler(buffer, playlist->title, NULL, 0);
    }

    arrdstry(playlists, sc_playlist_free_void);
}

static void sc_populate_tracks(sc_t *info, void *buffer, fuse_fill_dir_t filler) {
   const array_t *tracks = sc_get_tracks(info); 
   
   for (int i = 0; i < tracks->length; i++) {
       sc_track_t *track = (sc_track_t*)arrget(tracks, i);
       STR_REPLACE_CHAR(track->title, '/', '-');
       filler(buffer, track->title, NULL, 0);
   }

   arrdstry(tracks, sc_track_free_void);
}

static void sc_populate_playlist(sc_t *info, char *name, void *buffer,
                                 fuse_fill_dir_t filler) {
    const array_t *playlists = sc_get_playlists(info);

    for (int i = 0; i < playlists->length; i++) {
        sc_playlist_t *list = arrget(playlists, i);
        if (!strcmp(list->title, name)) {
            for (int j = 0; j < list->tracks->length; j++) {
                sc_track_t *track = arrget(list->tracks, j);
                filler(buffer, track->title, NULL, 0);
            }
        }
    }
    arrdstry(playlists, sc_playlist_free_void);
}

static void sc_populate_search(sc_t *info, char *q, void *buffer, 
                               fuse_fill_dir_t filler) {
    const array_t *tracks = sc_search_tracks(info, q);

    for (int i = 0; i < tracks->length; i++) {
        sc_track_t *track = arrget(tracks, i);
        STR_REPLACE_CHAR(track->title, '/', '-');
        filler(buffer, track->title, NULL, 0);
    }
    
    arrdstry(tracks, sc_track_free_void);
}

/**
 * path: result of calling sc_split_path(path) 
 * return: desired action based on path
 */
static enum sc_action_e sc_get_action(array_t *path) {
    if (path->length == 0) {
        return ROOT;
    } else if (path->length == 1) {
        if (!strcmp(arrget(path, 0), "me")) {
            return ME;
        }
    } else if (path->length == 2) {
        if (!strcmp(arrget(path, 1), "tracks")) {
            return TRACKS;
        } else if (!strcmp(arrget(path, 1), "playlists")) {
            return PLAYLISTS;
        }
    } else if (path->length == 3) {
        if (!strcmp(arrget(path, 1), "playlists")) {
            return PLAYLIST;
        } else if (!strcmp(arrget(path, 1), "tracks")) {
            return TRACK;
        }
    } else if (path->length == 4) {
        if (!strcmp(arrget(path, 1), "playlists")) {
            return TRACK_FROM_PLAYLIST;
        }
    }
    return -1;
}

static bool sc_get_track_from_user(sc_t *info, array_t *elems, sc_track_t *ret) {
    bool found = false;
    const array_t *tracks = sc_get_tracks(info);

    for (int i = 0; i < tracks->length; i++) {
        sc_track_t *track = (sc_track_t*)arrget(tracks, i);
        if (!strcmp(track->title, arrget(elems, 2))) {
            sc_track_cpy(ret, track);
            found = true;
            break;
        }
    }
    
    arrdstry(tracks, sc_track_free_void);
    return found;
}

static bool sc_get_track_from_playlist(sc_t *info, array_t *elems, sc_track_t *ret) {
    bool found = false;
    const array_t *playlists = sc_get_playlists(info);

    for (int i = 0; i < playlists->length; i++) {
        sc_playlist_t *list = arrget(playlists, i);
        if (!strcmp(list->title, arrget(elems, 2))) {
            for (int j = 0; j < list->tracks->length; j++) {
                sc_track_t *track = arrget(list->tracks, j);
                if (!strcmp(track->title, arrget(elems, 3))) {
                    sc_track_cpy(ret, track); 
                    found = true;
                    break;
                }
            }
        }
    }
    arrdstry(playlists, sc_playlist_free_void);
    return found;
}

static int sc_getattr(const char *path, struct stat* info) {
    int res = 0;

    memset(info, 0, sizeof(struct stat));

    if (!strcmp(path, "/")) {
        info->st_mode = S_IFDIR | 0775;
        info->st_nlink = 2;
    } else if (sc_is_dir(path)) {
        info->st_mode = S_IFDIR | 0544;
        info->st_nlink = 1;
    } else {
        info->st_mode = S_IFREG | 0440;
        info->st_nlink = 1;
        sc_track_t *tmp = sc_get_song((sc_t*)fuse_get_context()->private_data, path);
        if (!tmp) {
            res = -ENOENT;
        } else {
            info->st_size = tmp->data->size;
            info->st_mtime = tmp->last_mod;
            sc_track_free(tmp);
        }
    }

    return res;
}

static int sc_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

/**
 * handle the various cases where a user can get a track stream and then call
 * the appropriate function
 */
static sc_track_t *sc_get_song(sc_t *info, const char *path) {

    array_t *elems = sc_split_path(path);

    char *save = info->username;

    if (strcmp(arrget(elems, 0), "me")) {
        info->username = arrget(elems, 0);
    }

    sc_track_t *ret = malloc(sizeof(sc_track_t));

    enum sc_action_e act = sc_get_action(elems);

    bool found = false;

    switch (act) {
        case TRACK:
            found = sc_get_track_from_user(info, elems, ret);
            break;
        case TRACK_FROM_PLAYLIST:
            found = sc_get_track_from_playlist(info, elems, ret);
            break;
        default:
            break;
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

    if (args->length > 0 && strcmp(arrget(args, 0), "me")) {
        info->username = arrget(args, 0);
    }

    enum sc_action_e action = sc_get_action(args);

    
    switch (action) {
        case ROOT:
            sc_populate_root(buffer, filler);
            break;
        case ME:
            sc_populate_me(buffer, filler);
            break;
        case TRACKS: 
            sc_populate_tracks(info, buffer, filler);
            break;
        case PLAYLISTS:
            sc_populate_playlists(info, buffer, filler);
            break;
        case PLAYLIST:
            sc_populate_playlist(info, arrget(args, 2), buffer, filler);
            break;
        case SEARCH:
            sc_populate_search(info, arrget(args, 1), buffer, filler);
            break;
        default:
            break;
    }

    // for navigaiton
    filler(buffer, ".", NULL, 0);
    filler(buffer, "..", NULL, 0);

    // don't eliminate the original username
    info->username = save;

    arrdstryfree(args);
    return 0;
}

static int sc_releasedir(const char *path, struct fuse_file_info *fi) {
    return 0;
}

// cleanup
static void sc_destroy(void *buffer) {
    sc_t *info = (sc_t*)fuse_get_context()->private_data; 
    sc_free(info);
    syslog(LOG_INFO | LOG_USER, "exiting scfs");
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
    syslog(LOG_INFO | LOG_USER, "starting scfs");
    argv[--argc] = NULL;
    argv[--argc] = NULL;
    return fuse_main(argc, argv, &sc_ops, sc_info);
}
