#include <assert.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdlib.h>

#include "cgc.h"

static void ll_insert(cgc_t* gc, hdr_t* h) {
    h->next = gc->allocs;
    h->prev = NULL;
    if (gc->allocs)
        gc->allocs->prev = h;
    gc->allocs = h;
}

static void ll_remove(cgc_t* gc, hdr_t* h) {
    if (h->next)
        h->next->prev = h->prev;
    if (h->prev)
        h->prev->next = h->next;
    else
        gc->allocs = h->next;
}

void cgc_new(cgc_t* gc, char* stack_bottom) {
    *gc = (cgc_t){
        .allocs = NULL,
        .stack_bottom = stack_bottom,
    };
}

void* cgc_malloc(cgc_t* gc, size_t size) {
    hdr_t* h = (hdr_t*) malloc(size + sizeof(hdr_t));
    if (!h) {
        return NULL;
    }
    h->sz = size;
    ll_insert(gc, h);

    return (void*) (h + 1);
}

void* cgc_calloc(cgc_t* gc, size_t num, size_t size) {
    hdr_t* h = (hdr_t*) calloc(num * size + sizeof(hdr_t), 1);
    if (!h) {
        return NULL;
    }
    h->sz = num * size;
    ll_insert(gc, h);

    return (void*) (h + 1);
}

void* cgc_realloc(cgc_t* gc, void* ptr, size_t size) {
    hdr_t* oldh = (hdr_t*) ptr - 1;
    hdr_t* h = (hdr_t*) realloc(oldh, size + sizeof(hdr_t));
    if (!h) {
        return NULL;
    }
    ll_remove(gc, oldh);
    h->sz = size;
    ll_insert(gc, h);

    return (void*) (h + 1);
}

void cgc_free(cgc_t* gc, void* ptr) {
    hdr_t* h = (hdr_t*) ptr - 1;
    ll_remove(gc, h);
    free(h);
}

static hdr_t* find(cgc_t* gc, uintptr_t ptr) {
    hdr_t* h = gc->allocs;
    while (h) {
        uintptr_t start = (uintptr_t) (h + 1);
        if (ptr >= start && ptr < start + h->sz) {
            return h;
        }
        h = h->next;
    }
    return NULL;
}

static void mark(cgc_t* gc, char* base, size_t sz) {
    const size_t ptrsz = sizeof(void*);

    if (sz < ptrsz || (uintptr_t) base + sz < (uintptr_t) base) {
        return;
    }

    for (size_t i = 0; i <= sz - ptrsz; i += ptrsz) {
        void* maybe_ptr;
        memcpy(&maybe_ptr, &base[i], ptrsz);
        hdr_t* h = find(gc, (uintptr_t) maybe_ptr);
        if (h && !h->mark) {
            h->mark = true;
            mark(gc, (char*) (h + 1), h->sz);
        }
    }
}

static uintptr_t stack_top() {
    int x;
    return (uintptr_t) &x;
}

static uintptr_t get_stack_top() {
    jmp_buf env;
    setjmp(env);
    uintptr_t (*volatile f)() = stack_top;
    return f();
}

static void mark_all(cgc_t* gc) {
    char* stack_top = (char*) get_stack_top();
    mark(gc, stack_top, gc->stack_bottom - stack_top);
}

static void sweep_collect(cgc_t* gc) {
    hdr_t* h = gc->allocs;
    while (h) {
        if (!h->mark) {
            cgc_free(gc, h + 1);
        }
        h->mark = false;
        h = h->next;
    }
}

void cgc_collect(cgc_t* gc) {
    mark_all(gc);
    sweep_collect(gc);
}
