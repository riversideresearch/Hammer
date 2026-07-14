/* Copyright (c) 2026 Riverside Research */

#include "static_alloc.h"
#include "allocator.h"
#include <stdio.h>

#ifdef RTEMS_BUILD
#include <rtems.h>
#define SA_HALT(fmt, ...) do { printf("static_alloc: " fmt "\n", ##__VA_ARGS__); \
                               rtems_fatal(RTEMS_FATAL_SOURCE_APPLICATION, 1); } while (0)
#else
#include <stdio.h>
#include <stdlib.h>
#define SA_HALT(fmt, ...) do { fprintf(stderr, "static_alloc: " fmt "\n", ##__VA_ARGS__); \
                               abort(); } while (0)
#endif

#include <stdint.h> 

// Change these to be bound to what is needed for grammar. More than likely that implies increasing Parse Pool
#define GRAMMAR_POOL_SIZE  (256u * 1024u)        /* 256 KB */
#define PARSE_POOL_SIZE    (256u * 1024u)  /* 256 MB   */

// SPARC Alignment
#define SA_ALIGN 8u

typedef struct {
    HAllocator  base; //keep first
    uint8_t    *buf;
    size_t      size;
    size_t      offset;
    size_t      high_water;
    const char *name;
} StaticAllocator;

static _Alignas(SA_ALIGN) uint8_t grammar_buf[GRAMMAR_POOL_SIZE];
static _Alignas(SA_ALIGN) uint8_t parse_buf[PARSE_POOL_SIZE];

static StaticAllocator grammar_sa;
static StaticAllocator parse_sa;

/* Public handles callers reference. Set in h_static_alloc_init. */
HAllocator *h_grammar_allocator = NULL;
HAllocator *h_parse_allocator   = NULL;

/* Round a size up to the next SA_ALIGN multiple. Keeps the offset aligned
 * after each alloc, so the next allocation also starts aligned. */
static inline size_t sa_round_up(size_t n) {
    return (n + (SA_ALIGN - 1)) & ~((size_t)(SA_ALIGN - 1));
}

/* ---- HAllocator vtable ---- */

static void *sa_alloc(HAllocator *mm, size_t size) {
    StaticAllocator *a = (StaticAllocator *)mm;

    if (size == 0)
        size = 1;
    size = sa_round_up(size);

    if (a->offset + size > a->size) {
        // No heap to fall back to. Halt rather than corrupt
        SA_HALT("%s pool exhausted: need %lu, %lu free (size=%lu)",
                a->name,
                (unsigned long)size,
                (unsigned long)(a->size - a->offset),
                (unsigned long)a->size);
        return NULL; 
    }

    void *ret = a->buf + a->offset;
    a->offset += size;
    if (a->offset > a->high_water)
        a->high_water = a->offset;
    return ret;
}

static void *sa_realloc(HAllocator *mm, void *ptr, size_t size) {
    StaticAllocator *a = (StaticAllocator *)mm;
    (void)ptr;
    (void)size;
    // Not reachable on the single-shot h_parse path. If we get here, the
    // streaming (chunked) path was used unexpectedly. Fail
    SA_HALT("%s: realloc called but not supported on this path", a->name);
    return NULL; // unreachable
}

static void sa_free(HAllocator *mm, void *ptr) {
    (void)mm;
    (void)ptr;
    // no-op: bump allocators reclaim only by reset
}

// setup

void h_static_alloc_init(void) {
    grammar_sa.base.alloc   = sa_alloc;
    grammar_sa.base.realloc = sa_realloc;
    grammar_sa.base.free    = sa_free;
    grammar_sa.buf          = grammar_buf;
    grammar_sa.size         = GRAMMAR_POOL_SIZE;
    grammar_sa.offset       = 0;
    grammar_sa.high_water   = 0;
    grammar_sa.name         = "grammar";

    parse_sa.base.alloc     = sa_alloc;
    parse_sa.base.realloc   = sa_realloc;
    parse_sa.base.free      = sa_free;
    parse_sa.buf            = parse_buf;
    parse_sa.size           = PARSE_POOL_SIZE;
    parse_sa.offset         = 0;
    parse_sa.high_water     = 0;
    parse_sa.name           = "parse";

    h_grammar_allocator = &grammar_sa.base;
    h_parse_allocator   = &parse_sa.base;
}

void h_parse_pool_reset(void) {
    parse_sa.offset = 0;
}

size_t h_grammar_pool_used(void)     { return grammar_sa.offset; }
size_t h_parse_pool_used(void)       { return parse_sa.offset; }
size_t h_parse_pool_high_water(void) { return parse_sa.high_water; }