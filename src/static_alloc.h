/* Copyright (c) 2026 Riverside Research */

#ifndef HAMMER_STATIC_ALLOC_H
#define HAMMER_STATIC_ALLOC_H

#include "allocator.h"   /* HAllocator */
#include <stddef.h>

/* Initialize the allocators. Call once, before grammar construction. */
void h_static_alloc_init(void);

/* The two allocators. Pass their addresses wherever Hammer wants HAllocator*. */
extern HAllocator *h_grammar_allocator;
extern HAllocator *h_parse_allocator;

/** @brief Reset the per-parse pool to empty. Call after each h_parse, once you are
 * done reading the HParseResult (reset invalidates everything the parse
 * allocated, including the result). */
void h_parse_pool_reset(void);

// checks for sizing and such
size_t h_grammar_pool_used(void);
size_t h_parse_pool_used(void);
size_t h_parse_pool_high_water(void);  // max across all parses since start

#endif /* HAMMER_STATIC_ALLOC_H */