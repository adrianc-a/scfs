/**
 * Author: Adrian Chmielewski-Anders
 * Date: 2 May 2015
 */

#include <time.h>

#include "sc.h"
#include "utils.h"
#include "http.h"

/**
 * Create url for soundcloud api.
 * endpoint: one of SC_E... i.e. "/users"
 * sub: subresource i.e. "/playlists"
 * id: id of resource i.e. user_id, track_id etc, may be NULL
 * len: number of varargs, use SC_ARG_LEN(len) or add 2 because of the extra
 *      client id arg/value pair
 * args: ALL must be char* in form name, val -> "name=val"
 */
static char *sc_create_url(char *endpoint, char *sub, char *id, int len, 
        const char *args[]) {
    assert(endpoint);
    assert(!(len % 2));
    assert(!(!id && sub));
    
    // memory for http://soundcloud.com/users and \0 at this point
    size_t mem = strlen(SC_BASE_URL) + strlen(endpoint) + 1;

    if (sub) {
        // for "/subresource"
        mem += strlen(sub);
    }

    if (id) {
        // for "/5423534.json"
        mem += 1 + strlen(id) + strlen(SC_RTYPE);
    }

    char *ret = malloc(mem);

    // !id && sub is not possible
    if (sub && id) {
        sprintf(ret, "%s%s%s%s%s%s", SC_BASE_URL, endpoint, "/", id, sub,
                SC_RTYPE);
    } else if (!sub && id) {
        sprintf(ret, "%s%s%s%s%s", SC_BASE_URL, endpoint, "/", id, SC_RTYPE);
    } else if (!sub && !id) {
        sprintf(ret, "%s%s%s", SC_BASE_URL, endpoint, SC_RTYPE);
    }
    
    ret[mem - 1] = '\0';

    http_create_url(&ret, len, args);

    return ret;
}

static struct tm sc_get_time_from_str(char *time_stamp) {
    struct tm t;
    memset(&t, 0, sizeof(struct tm));
    strptime(time_stamp, "%Y/%m/%d %H:%M:%S +0000", &t);
    return t;
}

sc_t *sc_new(char *uname, char *cid) {
    sc_t *ret = malloc(sizeof(sc_t));
    ret->username = strdup(uname);
    ret->client_id = strdup(cid);

    assert(ret->username);
    assert(ret->client_id);

    return ret;
}

void sc_free(sc_t *sc) {
    free(sc->username);
    free(sc->client_id);
    free(sc);
}

sc_track_t *sc_track_new(char *i, char *t, buf_t *contents) {
    sc_track_t *ret = malloc(sizeof(sc_track_t));
    ret->id = strdup(i);
    ret->title = strdup(t);
    ret->data = malloc(sizeof(buf_t));
    buf_cpy(ret->data, contents);

    return ret;
}

sc_track_t *sc_track_from_json(cJSON *json) {
    // if we cannot stream it, don't even process it
    if (!cJSON_GetObjectItem(json, "streamable")->valueint) {
        return NULL;
    }

    char *id;
    I_TO_A(cJSON_GetObjectItem(json, "id")->valueint, id);
    char *title = cJSON_GetObjectItem(json, "title")->valuestring;
    char *time_stamp = cJSON_GetObjectItem(json, "last_modified")->valuestring;
    struct tm t = sc_get_time_from_str(time_stamp);

    // todo, fix this
    size_t o_size = (size_t)cJSON_GetObjectItem(json, 
            "original_content_size")->valueint;

    buf_t tmp = {
        .size = o_size,
        .data = NULL,
        .allocated = 0
    };

    sc_track_t *ret = sc_track_new(id, title, &tmp);
    ret->last_mod = tm_to_epoch(&t);
    free(id);
    return ret;
}

void sc_track_cpy(sc_track_t *dst, sc_track_t *src) {

    dst->title = strdup(src->title);
    dst->id = strdup(src->id);
    dst->data = malloc(sizeof(buf_t));
    dst->last_mod = src->last_mod;
    buf_cpy(dst->data, src->data);
}


bool sc_track_equals(sc_track_t *a, sc_track_t *b) {
    if (a == b) return true;

    if (!a | !b) return false;

    if (!strcmp(a->id, b->id)) {
        return true;
    }

    return false;
}
void sc_track_free(sc_track_t *track) {
    free(track->id);
    free(track->title);
    buf_free(track->data);
    free(track);
}

void sc_track_free_void(void *track) {
    sc_track_t *tr = (sc_track_t*)track;
    sc_track_free(tr);
}

sc_playlist_t *sc_playlist_new(char *i, char *t, array_t *tr) {
    sc_playlist_t *ret = malloc(sizeof(sc_playlist_t));
    ret->id = strdup(i);
    ret->title = strdup(t);
    ret->tracks = tr;

    return ret;
}

sc_playlist_t* sc_playlist_from_json(cJSON *json) {
    // json array
    cJSON *tracks_json = cJSON_GetObjectItem(json, "tracks");
    array_t *songs = arrnews(cJSON_GetArraySize(tracks_json));

    for (int i = 0; i < cJSON_GetArraySize(tracks_json); i++ ) {
        cJSON *cur = cJSON_GetArrayItem(tracks_json, i);
        sc_track_t *track = sc_track_from_json(cur);
        if (track) {
            arradd(songs, (void*)track);
        }
    }
    char *id_buffer;
    I_TO_A(cJSON_GetObjectItem(json, "id")->valueint, id_buffer);
    char *time_str = cJSON_GetObjectItem(json, "last_modified")->valuestring;

    sc_playlist_t* ret = sc_playlist_new(
            id_buffer,
            cJSON_GetObjectItem(json, "title")->valuestring,
            songs);
    struct tm t = sc_get_time_from_str(time_str);
    ret->last_mod = tm_to_epoch(&t);
    return ret;
}

void sc_playlist_free(sc_playlist_t *playlist) {
    free(playlist->id);
    free(playlist->title);
    arrdstry(playlist->tracks, sc_track_free_void);
    free(playlist);
}

void sc_playlist_free_void(void *playlist) {
    sc_playlist_free((sc_playlist_t*)playlist);
}

const array_t *sc_get_playlists(sc_t *sc) {
    char *url = sc_create_url(SC_EUSERS, SC_EPLAYLISTS, sc->username, 
                              SC_ARG_LEN(0),
                              SC_DEFAULT_ARG_LIST(sc->client_id));

    cJSON *root = http_get_json(url);
    free(url);
    array_t *playlists = arrnew(cJSON_GetArraySize(root));

    for (int i = 0; i < cJSON_GetArraySize(root); i++) {
        cJSON *playlist = cJSON_GetArrayItem(root, i);

        arradd(playlists, (void*)sc_playlist_from_json(playlist));
    }

    cJSON_Delete(root);

    return playlists;
}

const array_t *sc_tracks_from_json(cJSON *root) {
    int len = cJSON_GetArraySize(root);
    array_t *tracks = arrnew();

    for (int i = 0; i < len; i++) {
        cJSON *track_json = cJSON_GetArrayItem(root, i);
        sc_track_t *track = sc_track_from_json(track_json);
        if (track) {
            arradd(tracks, (void*)track);
        }
    }

    cJSON_Delete(root);
    return tracks;
}

const array_t *sc_get_tracks(sc_t *sc) {
    char *url = sc_create_url(SC_EUSERS, SC_ETRACKS, sc->username,
                              SC_ARG_LEN(0), 
                              SC_DEFAULT_ARG_LIST(sc->client_id));

    cJSON *root = http_get_json(url);
    free(url);

    return sc_tracks_from_json(root);
}

void *sc_get_track_stream(sc_t *sc, sc_track_t *track) {
    char *url = sc_create_url(SC_ETRACKS, SC_ESTREAM, track->id,
                              SC_ARG_LEN(0),
                              SC_DEFAULT_ARG_LIST(sc->client_id));

    buf_t *buffer = http_get(url);
    free(url);
    
    track->data = buffer;
}

const array_t *sc_search_tracks(sc_t *sc, char *q) {
    char *url = sc_create_url(SC_ETRACKS, NULL, NULL, SC_ARG_LEN(2),
            SC_ARG_LIST(sc->client_id, "q", q, "limit", "10"));
    cJSON *root = http_get_json(url);

    free(url);

    return sc_tracks_from_json(root);
}
