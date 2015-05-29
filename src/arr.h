/**
 * arr.h
 * Author: Adrian Chmielewski-Anders
 * Date: 30 November 2014
 */

#ifndef ARR_H
#define ARR_H

#define int_val *(int*)
#define long_val *(long*)
#define str_val (char*)
#define char_val *(char*)
#define float_val *(float*)
#define double_val *(double*)

enum type_e {
    INT, LONG, CHAR, STR, FLOAT, DOUBLE, VOID, OTHER
};


/**
 * length: how many elements are in the array
 * size: how many elements have been allocated, not in bytes but just spaces for
 * void* that have been allocated
 * vaues: array of void*'s
 */
typedef struct {
    int length, size;
    void** values;
} array_t;

array_t* arrnews(int initial);
array_t* arrnew();
void arrdstry(array_t* arr, void (*dstry)(void*));
void arrdstryfree(array_t* arr);

void* arrget(const array_t* arr, int index);
void arradd(array_t* arr, void* value);
void arrset(array_t*, void* value, int index);
void* arrrm(array_t* arr, int index);
int arrindexof(array_t* arr, void* val);
int arrindexofval(array_t* arr, void* val, int (*pcmp)(void*, void*));
void* arrrmval(array_t* arr, void* val);
void arrprnt(array_t*, enum type_e, void (*prntfunc)(void*));

#endif // ARR_H
