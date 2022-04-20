#pragma once

#include <stdbool.h>
#include <string.h>

typedef struct hdr {
    size_t sz;
    bool mark;

    struct hdr* next;
    struct hdr* prev;
} hdr_t;

typedef struct {
    hdr_t* allocs;
    char* stack_bottom;
} cgc_t;

void cgc_new(cgc_t* gc, char* stack_bottom);
void* cgc_malloc(cgc_t* gc, size_t size);
void* cgc_calloc(cgc_t* gc, size_t num, size_t size);
void* cgc_realloc(cgc_t* gc, void* ptr, size_t size);
void cgc_free(cgc_t* gc, void* ptr);
void cgc_collect(cgc_t* gc);
