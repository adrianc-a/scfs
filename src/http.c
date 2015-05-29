/**
 * Author: Adrian Chmielewski-Anders
 * Date: 2 May 2015
 */

#include "http.h"
#include "buf.h"
#include <string.h>

static size_t 
write_buf(void *contents, size_t size, size_t nmemb, void *userp) {
    buf_t *buf = (buf_t*)userp;
    size_t total = size * nmemb;

    buf_concat(buf, contents, total);
    return total;
}


buf_t *http_get(char *url) {
    CURL *handle;
    buf_t *buf = buf_new();

    curl_global_init(CURL_GLOBAL_DEFAULT);
    handle = curl_easy_init();

    curl_easy_setopt(handle, CURLOPT_URL, url);

    // soundcloud api redirects for mp3 streams
    curl_easy_setopt(handle, CURLOPT_FOLLOWLOCATION, 1L);

    //curl_easy_setopt(handle, CURLOPT_SSL_VERIFYPEER, 1L);
    //curl_easy_setopt(handle, CURLOPT_SSL_VERIFYHOST, 1L);

    curl_easy_setopt(handle, CURLOPT_WRITEFUNCTION, write_buf);
    curl_easy_setopt(handle, CURLOPT_WRITEDATA, (void*)buf);
    curl_easy_setopt(handle, CURLOPT_USERAGENT, AGENT);

    CURLcode resp = curl_easy_perform(handle);

    if (resp != CURLE_OK) {
        fprintf(stderr, "curl_easy_perform() failed %s\n",
                curl_easy_strerror(resp));
        assert(resp != CURLE_OK);
    }
    curl_easy_cleanup(handle);
    curl_global_cleanup();

    return buf;
}

cJSON *http_get_json(char *url) {
    buf_t *buffer = http_get(url);
    char *str = buf_to_str(buffer);
    buf_free(buffer);

    cJSON *ret = cJSON_Parse(str);
    free(str);
    return ret;
}

void http_create_url(char **base, int len, const char *args[]) {
    assert(base);
    assert(len);
    assert(!(len % 2));

    size_t mem = strlen(*base);

    for (int i = 0; i < len; i++) {
       mem += strlen(args[i]);
    }

    // 1 for \0 at end, num because for every 2 args in ...
    // there is a ?/& and =
    mem += 1 + len;


    *base = realloc(*base, mem);


    for (int i = 0; i < len - 1; i+=2) {
        if (!i) {
            strcat(*base, "?");
        } else {
            strcat(*base, "&");
        }

        strcat(*base, args[i]);
        strcat(*base, "=");
        strcat(*base, args[i + 1]);
    }
}
