/**
 * Author: Adrian Chmielewski-Anders
 * Date: 2 May 2015
 */

#ifndef SC_H
#define SC_H

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "arr.h"
#include "buf.h"
#include "cJSON.h"

extern char *strdup(const char *s);

typedef struct {
    char *username;
    char *client_id;
} sc_t;

sc_t *sc_new(char *uname, char *cid);
void sc_free(sc_t *sc);

// http/api operations

#define SC_BASE_URL "http://api.soundcloud.com"
#define SC_EUSERS "/users"
#define SC_ETRACKS "/tracks"
#define SC_EPLAYLISTS "/playlists"
#define SC_ESTREAM "/stream"

#define SC_RTYPE ".json"

#define SC_ARG_LIST(_id, ...) ((const char*[]){"client_id", _id, __VA_ARGS__})
#define SC_DEFAULT_ARG_LIST(_id) ((const char*[]){"client_id", _id})
#define SC_ARG_LEN(_length) (_length + 2)

typedef struct {
    char *id;
    char *title;
    buf_t *data; // data is initially NULL
    time_t last_mod;
} sc_track_t;

sc_track_t *sc_track_new(char *i, char *t, buf_t *contents);
sc_track_t *sc_track_from_json(cJSON *json);
void sc_track_cpy(sc_track_t *dst, sc_track_t *src);
bool sc_track_equals(sc_track_t *a, sc_track_t *b);
void sc_track_free(sc_track_t *track);
void sc_track_free_void(void *track);

typedef struct {
    char *id;
    char *title;
    array_t *tracks; // array of track_t* 
    time_t last_mod;
} sc_playlist_t;

sc_playlist_t *sc_playlist_new(char *i, char *t, array_t *tr);
sc_playlist_t *sc_playlist_from_json(cJSON *json);
void sc_playlist_free(sc_playlist_t *playlist);
void sc_playlist_free_void(void *playlist);


/**
 * Get a list of the users playlists, each playlist will contain it's tracks
 * return: array_t* of sc_playlist_t*
 */
const array_t *sc_get_playlists(sc_t *sc);

/**
 * Get a list of users tracks. Tracks which have streamable set to false will
 * not be in the returned array.
 * return: array_t* of track_t*
 */
const array_t *sc_get_tracks(sc_t *sc);

/**
 * Get the mp3 stream for a specific track. 
 */
void sc_get_track_stream(sc_t *sc, sc_track_t *track);

/**
 * Search soundcloud for tracks, limit to 10 tracks
 * q: query
 * return: array of sc_track_t* 's
 */
const array_t *sc_search_tracks(sc_t *sc, char *q);

#endif
