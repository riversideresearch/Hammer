#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

#include <glib.h>
#include <setjmp.h>
#include <string.h>

static void test_alloc_null_mm(void) { g_check_cmp_int(1, ==, 1); }
static void test_realloc(void) {
    void *ptr = h_alloc(&system_allocator, 10);
    g_check_cmp_ptr(ptr, !=, NULL);

    void *new_ptr = h_realloc(&system_allocator, ptr, 20);
    g_check_cmp_ptr(new_ptr, !=, NULL);

    system_allocator.free(&system_allocator, new_ptr);
}

static void test_new_arena_zero_size(void) {
    HArena *arena = h_new_arena(&system_allocator, 0);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {

        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);
        h_delete_arena(arena);
    }
}

static void test_new_arena_nonzero_size(void) {
    HArena *arena = h_new_arena(&system_allocator, 1024);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {

        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);
        h_delete_arena(arena);
    }
}

static void test_arena_malloc_fast_path(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {
        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);
        h_delete_arena(arena);
    }
}

static void test_arena_malloc_large_block(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {

        void *ptr = h_arena_malloc(arena, 5000);
        g_check_cmp_ptr(ptr, !=, NULL);
        h_delete_arena(arena);
    }
}

static void test_arena_malloc_new_block(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {

        void *ptr1 = h_arena_malloc(arena, 4000);
        g_check_cmp_ptr(ptr1, !=, NULL);

        void *ptr2 = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr2, !=, NULL);

        h_delete_arena(arena);
    }
}

static void test_arena_malloc_noinit(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {
        void *ptr = h_arena_malloc_noinit(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);
        h_delete_arena(arena);
    }
}

static void test_arena_set_except(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {
        jmp_buf buf;
        h_arena_set_except(arena, &buf);

        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);
        h_delete_arena(arena);
    }
}

static void test_arena_realloc(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {
        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);

        void *new_ptr = h_arena_realloc(arena, ptr, 200);
        g_check_cmp_ptr(new_ptr, !=, NULL);

        h_delete_arena(arena);
    }
}

static void test_allocator_stats(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {
        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);

        HArenaStats stats;
        h_allocator_stats(arena, &stats);
        g_check_cmp_int(stats.used, >=, 100);
        g_check_cmp_int(stats.wasted, >=, 0);

        h_delete_arena(arena);
    }
}

static void test_arena_free(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {
        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);

        h_arena_free(arena, ptr);

        h_delete_arena(arena);
    }
}

static void test_delete_arena(void) {
    HArena *arena = h_new_arena(&system_allocator, 4096);
    g_check_cmp_ptr(arena, !=, NULL);
    if (arena) {
        void *ptr = h_arena_malloc(arena, 100);
        g_check_cmp_ptr(ptr, !=, NULL);

        h_delete_arena(arena);
    }
}

/* Fake environment and fake allocator functions */
typedef struct {
    int alloc_called;
    int realloc_called;
    int free_called;
    void *last_ptr;
    size_t last_size;
} FakeEnv;

static void *fake_alloc(void *env, size_t size) {
    FakeEnv *e = env;
    e->alloc_called++;
    e->last_size = size;
    return (void *)0x11111111;
}

static void *fake_realloc(void *env, void *ptr, size_t size) {
    FakeEnv *e = env;
    e->realloc_called++;
    e->last_ptr = ptr;
    e->last_size = size;
    return (void *)0x22222222;
}

static void fake_free(void *env, void *ptr) {
    FakeEnv *e = env;
    e->free_called++;
    e->last_ptr = ptr;
}

static void test_wrap_sets_fields(void) {
    HAllocator a;
    HAllocatorVtable vt = {fake_alloc, fake_realloc, fake_free};
    FakeEnv env = {0};

    h_allocator_wrap(&a, &vt, &env);

    g_check_cmp_ptr(a.vt, !=, NULL);
    g_check_cmp_ptr(a.env, !=, NULL);
    g_check_cmp_ptr(a.alloc, !=, NULL);
    g_check_cmp_ptr(a.realloc, !=, NULL);
    g_check_cmp_ptr(a.free, !=, NULL);
}

static void test_alloc_dispatches(void) {
    HAllocator a;
    HAllocatorVtable vt = {fake_alloc, fake_realloc, fake_free};
    FakeEnv env = {0};

    h_allocator_wrap(&a, &vt, &env);

    void *p = a.alloc(&a, 64);
    g_check_cmp_ptr(p, ==, (void *)0x11111111);
    g_check_cmp_int(env.alloc_called, ==, 1);
    g_check_cmp_int(env.last_size, ==, 64);
}

static void test_realloc_dispatches(void) {
    HAllocator a;
    HAllocatorVtable vt = {fake_alloc, fake_realloc, fake_free};
    FakeEnv env = {0};

    h_allocator_wrap(&a, &vt, &env);

    void *p = a.realloc(&a, (void *)0xDEAD, 128);
    g_check_cmp_ptr(p, ==, (void *)0x22222222);
    g_check_cmp_int(env.realloc_called, ==, 1);
    g_check_cmp_ptr(env.last_ptr, ==, (void *)0xDEAD);
    g_check_cmp_int(env.last_size, ==, 128);
}

static void test_free_dispatches(void) {
    HAllocator a;
    HAllocatorVtable vt = {fake_alloc, fake_realloc, fake_free};
    FakeEnv env = {0};

    h_allocator_wrap(&a, &vt, &env);

    a.free(&a, (void *)0xBEEF);
    g_check_cmp_int(env.free_called, ==, 1);
    g_check_cmp_ptr(env.last_ptr, ==, (void *)0xBEEF);
}

void register_allocator_tests(void) {
    g_test_add_func("/core/allocator/alloc_null_mm", test_alloc_null_mm);
    g_test_add_func("/core/allocator/realloc", test_realloc);
    g_test_add_func("/core/allocator/new_arena_zero_size", test_new_arena_zero_size);
    g_test_add_func("/core/allocator/new_arena_nonzero_size", test_new_arena_nonzero_size);
    g_test_add_func("/core/allocator/arena_malloc_fast_path", test_arena_malloc_fast_path);
    g_test_add_func("/core/allocator/arena_malloc_large_block", test_arena_malloc_large_block);
    g_test_add_func("/core/allocator/arena_malloc_new_block", test_arena_malloc_new_block);
    g_test_add_func("/core/allocator/arena_malloc_noinit", test_arena_malloc_noinit);
    g_test_add_func("/core/allocator/arena_set_except", test_arena_set_except);
    g_test_add_func("/core/allocator/arena_realloc", test_arena_realloc);
    g_test_add_func("/core/allocator/allocator_stats", test_allocator_stats);
    g_test_add_func("/core/allocator/arena_free", test_arena_free);
    g_test_add_func("/core/allocator/delete_arena", test_delete_arena);
    g_test_add_func("/core/allocator/test_wrap_sets_fields", test_wrap_sets_fields);
    g_test_add_func("/core/allocator/test_alloc_dispatches", test_alloc_dispatches);
    g_test_add_func("/core/allocator/test_realloc_dispatches", test_realloc_dispatches);
    g_test_add_func("/core/allocator/test_free_dispatches", test_free_dispatches);
}
