/**
 * arr.c
 * Author: Adrian Chmielewski-Anders
 * Date: 30 November 2014
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arr.h"

array_t* arrnews(int initial) {
    array_t* ret = malloc(sizeof(array_t));
    ret->values = malloc(sizeof(void*) * initial);
    ret->length = 0;
    ret->size   = initial;
    for (int i = 0; i < initial; i++) {
        ret->values[i] = NULL;
    }
    return ret;
}

array_t* arrnew() {
    return arrnews(1);
}

void arrdstry(array_t* arr, void (*dstry)(void*)) {
    for (int i = 0; i < arr->length; i++) {
        if (arr->values[i]) {
            (*dstry)(arr->values[i]);
        }
    }
    free(arr->values);
    free(arr);
}

void arrdstryfree(array_t* arr) {
    arrdstry(arr, free);
}

void* arrget(const array_t* arr, int index) {
    if (index >= arr->length) {
        return NULL;
    }
    return arr->values[index];
}

/**
 * increase the *size* of the array by 2x
 */
static void grow(array_t* arr) {
    arr->size = arr->size * 2;
    void** tmp = malloc(sizeof(void*) * arr->size);
    for (int i = 0; i < arr->size; i++) {
        if (i < arr->length) {
            tmp[i] = arr->values[i];
        } else {
            tmp[i] = NULL; 
        }
    }
    free(arr->values);
    arr->values = tmp;
}

static void add(array_t* arr, void* value, int index) {
    if (index >= arr->size) {
        grow(arr);    
    } 
    arr->values[index] = value;
}

void arradd(array_t* arr, void* value) {
    add(arr, value, arr->length);
    arr->length++;
}

void arrset(array_t* arr, void* value, int index) {
    if (index >= arr->size * 2) {
        return;
    }
    add(arr, value, index);
}

void* arrrm(array_t* arr, int index) {
    if (index >= arr->length) {
        return NULL;
    }
    void* ref = arr->values[index];
    for (int i = index; i < arr->length - 1; i++) {
       arr->values[i] = arr->values[i + 1];
    }
    arr->values[arr->length - 1] = NULL;
    arr->length--;
    return ref;
}

int arrindexof(array_t* arr, void* val) {
    for (int i = 0; i < arr->length; i++) {
        if (arr->values[i] == val) {
            return i;
        }
    }
    return -1;
}

/**
 * pcmp is pointer to a proprietary compare function that take two pointers
 * and returns 0 if they are equal
 */
int arrindexofval(array_t* arr, void* val, int (*pcmp)(void*, void*)) {
    for (int i = 0; i < arr->length; i++) {
        if (!(*pcmp)(arr->values[i], val)) {
            return i;
        }
    }
    return -1;
}


void* arrrmval(array_t* arr, void* val) {
    return arrrm(arr, arrindexof(arr, val));
}

void arrprnt(array_t* arr, enum type_e cast, void (*prntfunc)(void*) ) {
    printf("length: %d size: %d [", arr->length, arr->size);
    int i;
    for (i = 0; i < arr->length; i++) {
        if (!i) {
            printf(" ");
        } else {
            printf(", ");
        }
        switch (cast) {
            case INT:
                printf("%d", int_val(arr->values[i])); 
                break;
            case LONG:
                printf("%ld", long_val(arr->values[i]));
                break;
            case CHAR:
                printf("%c", char_val(arr->values[i]));
            case STR:
                printf("%s", str_val(arr->values[i])); 
                break;
            case FLOAT:
                printf("%f", float_val(arr->values[i])); 
                break;
           case DOUBLE:
                printf("%lf", double_val(arr->values[i])); 
                break;
           case VOID:
                printf("%p", arr->values[i]);
                break;
           default:
                if (prntfunc) {
                    (*prntfunc)(arr->values[i]);
                }
                break;
        }
    }
    printf(" ]\n");
}
