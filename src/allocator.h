/* Copyright (c) 2026 Riverside Research */
/* Arena allocator for Hammer.
 * Copyright (C) 2012  Meredith L. Patterson, Dan "TQ" Hirsch
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation, version 2.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef HAMMER_ALLOCATOR__H__
#define HAMMER_ALLOCATOR__H__
#include <setjmp.h>
#include <sys/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined __llvm__
#if __has_attribute(malloc)
#define ATTR_MALLOC(n) __attribute__((malloc))
#else
#define ATTR_MALLOC(n)
#endif
#elif defined SWIG
#define ATTR_MALLOC(n)
#elif defined __GNUC__
#define ATTR_MALLOC(n) __attribute__((malloc, alloc_size(2)))
#else
#define ATTR_MALLOC(n)
#endif

/* #define DETAILED_ARENA_STATS */

// HAllocatorVtable: function pointers which take an environment pointer.
typedef struct HAllocatorVtable_ {
    void *(*alloc)(void *env, size_t size);
    void *(*realloc)(void *env, void *ptr, size_t size);
    void (*free)(void *env, void *ptr);
} HAllocatorVtable;

// Backwards-compatible HAllocator wrapper.
// The first three fields keep the old layout so static initializers
// that supply function pointers directly continue to work.
typedef struct HAllocator_ {
    /* legacy-style function pointers (kept first for static init compat) */
    void *(*alloc)(struct HAllocator_ *allocator, size_t size);
    void *(*realloc)(struct HAllocator_ *allocator, void *ptr, size_t size);
    void (*free)(struct HAllocator_ *allocator, void *ptr);

    /* new-style vtable and environment pointer */
    HAllocatorVtable *vt;
    void *env;
} HAllocator;

void *h_alloc(HAllocator *allocator, size_t size) ATTR_MALLOC(2);
void *h_realloc(HAllocator *allocator, void *ptr, size_t size);

typedef struct HArena_ HArena; // hidden implementation

HArena *h_new_arena(HAllocator *allocator, size_t block_size); // pass 0 for default...

// Initialize an HAllocator wrapper to dispatch to a vtable with the
// provided environment pointer. After calling this, use the returned
// HAllocator like the legacy allocator (pass it to functions expecting
// an HAllocator*).
void h_allocator_wrap(HAllocator *out, HAllocatorVtable *vt, void *env);

void *h_arena_malloc_noinit(HArena *arena, size_t count) ATTR_MALLOC(2);
void *h_arena_malloc(HArena *arena, size_t count) ATTR_MALLOC(2);
void *h_arena_realloc(HArena *arena, void *ptr, size_t count);
void h_arena_free(HArena *arena,
                  void *ptr); // For future expansion, with alternate memory managers.
void h_delete_arena(HArena *arena);
void h_arena_set_except(HArena *arena, jmp_buf *except);

typedef struct {
    size_t used;
    size_t wasted;
#ifdef DETAILED_ARENA_STATS
    size_t mm_malloc_count;
    size_t mm_malloc_bytes;
    size_t memset_count;
    size_t memset_bytes;
    size_t arena_malloc_count;
    size_t arena_malloc_bytes;
    /* small, uninited */
    size_t arena_su_malloc_count;
    size_t arena_su_malloc_bytes;
    /* small, inited */
    size_t arena_si_malloc_count;
    size_t arena_si_malloc_bytes;
    /* large, uninited */
    size_t arena_lu_malloc_count;
    size_t arena_lu_malloc_bytes;
    /* large, inited */
    size_t arena_li_malloc_count;
    size_t arena_li_malloc_bytes;
#endif
} HArenaStats;

void h_allocator_stats(HArena *arena, HArenaStats *stats);

#ifdef __cplusplus
}
#endif

#endif // #ifndef LIB_ALLOCATOR__H__
