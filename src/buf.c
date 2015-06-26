/**
 * Author: Adrian Chmielewski-Anders
 * Date: 1 May 2015
 */

#include "buf.h"
#include <assert.h>
#include <string.h>

buf_t *buf_new() {
    buf_t *ret = malloc(sizeof(buf_t));
    ret->data      = malloc(INIT_SIZE);
    ret->size      = 0;
    ret->allocated = INIT_SIZE;
    return ret;
}

void buf_cpy(buf_t *dst, buf_t *src) {
    if (!src) return;
    dst->data = malloc(src->allocated);

    dst->allocated = src->allocated;
    dst->size = src->size;

    if (src->allocated && src->data && dst->data) {
        memcpy(dst->data, src->data, src->size);
    }
}

void buf_free(buf_t *b) {
    if (b) {
        free(b->data);
        free(b);
    }
}

void buf_add(buf_t *b, uint8_t a) {
    assert(b); 

    if (b->size >= b->allocated) {
        b->allocated = b->allocated * 2;
        b->data = realloc(b->data, b->allocated);
    }
    b->data[b->size++] = a;
}

void buf_concat(buf_t *dst, uint8_t *src, size_t len) {
    for (size_t i = 0; i < len; i++) {
        buf_add(dst, src[i]);
    }
}

char* buf_to_str(buf_t *b) {
   char *ret = malloc(b->size + 1);
   memcpy(ret, b->data, b->size);
   ret[b->size] = '\0';
   return ret;
}
