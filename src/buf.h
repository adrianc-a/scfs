/**
 * Author: Adrian Chmielewski-Anders
 * Date: 1 May 2015
 */

#ifndef BUF_H
#define BUF_H

#include <stdlib.h>
#include <stdint.h>

/* initially data has INIT_SIZE bytes, relatively large because 
 * meant for http reqs so HTML/JSON can be rather large
 */
#define INIT_SIZE 100

typedef struct {
    size_t size;      // size in bytes
    size_t allocated; // size allocated
    uint8_t *data; 
} buf_t;

buf_t *buf_new();
void buf_cpy(buf_t *dst, buf_t *src);
void buf_free(buf_t *b);

void buf_add(buf_t *b, uint8_t a);
void buf_concat(buf_t *dst, uint8_t *src, size_t len);
char *buf_to_str(buf_t *b);

#endif
