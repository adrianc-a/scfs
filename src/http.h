/**
 * Author: Adrian Chmielewski-Anders
 * Date: 2 May 2015
 */

#ifndef HTTP_H
#define HTTP_H

#include <stdlib.h>
#include <curl/curl.h>
#include <assert.h>
#include <string.h>
#include "buf.h"
#include "cJSON.h"

#define AGENT "libcurl-agent/1.0"

#define ARG_LIST(...) ((const char*[]){__VA_ARGS__})

/**
 * url: url
 * return: buf_t, see buf.c if need string use buf_to_str(buf_t*) make sure to
 *         clean up with buf_free(buf_t*)
 */
buf_t *http_get(char *url);
cJSON *http_get_json(char *url);

/**
 * Create a url from a base (i.e. http://example.com) and
 * append the http params so it's http://example.com?arg1=val1&arg2=val2
 * base: address of base url (i.e. http://example.com)
 * len: number of arguments that follow, must be even
 * ...: args in form name, val becomes name=val, all arguments are assumed to be
 *      valid, non-NULL c-strings properly null terminated.
 */
void http_create_url(char **base, int len, const char *args[]);

#endif
