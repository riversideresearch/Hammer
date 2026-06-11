#include "hammer.h"
#include "sloballoc.h"
#include "test_suite.h"

#include <glib.h>
#include <setjmp.h>
#include <signal.h>
#include <stdint.h>

static jmp_buf abort_jmp_buf;
// Signal handler for testing abort conditions
/*
static void abort_handler(int sig) {
    (void)sig;
    longjmp(abort_jmp_buf, 1);
}*/

static void test_slobinit(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    g_check_cmp_ptr(slob, !=, NULL);
    if (slob) {
        void *ptr = sloballoc(slob, 100);
        g_check_cmp_ptr(ptr, !=, NULL);
    }
}

static void test_sloballoc_small_size(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 1);
    g_check_cmp_ptr(ptr, !=, NULL);
}

static void test_sloballoc_overflow(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, SIZE_MAX);
    g_check_cmp_ptr(ptr, ==, NULL);
}

static void test_sloballoc_remblock(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 100);
    g_check_cmp_ptr(ptr, !=, NULL);
}

static void test_sloballoc_exact_fit(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 900);
    g_check_cmp_ptr(ptr, !=, NULL);
}

static void test_slobfree_left_adjacent(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr1 = sloballoc(slob, 100);
    void *ptr2 = sloballoc(slob, 100);
    slobfree(slob, ptr1);
    slobfree(slob, ptr2);
}

static void test_slobfree_right_adjacent(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr1 = sloballoc(slob, 100);
    void *ptr2 = sloballoc(slob, 100);
    slobfree(slob, ptr2);
    slobfree(slob, ptr1);
}

static void test_slobfree_both_adjacent(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr1 = sloballoc(slob, 100);
    void *ptr2 = sloballoc(slob, 100);
    void *ptr3 = sloballoc(slob, 100);
    slobfree(slob, ptr1);
    slobfree(slob, ptr3);
    slobfree(slob, ptr2);
}

static void test_slobfree_no_adjacent(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr1 = sloballoc(slob, 100);
    void *ptr2 = sloballoc(slob, 200);
    slobfree(slob, ptr1);
    slobfree(slob, ptr2);
}

static void test_slobcheck(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    int result = slobcheck(slob);
    g_check_cmp_int(result, ==, 0);
}

static void test_h_sloballoc(void) {
    uint8_t mem[1024];
    HAllocator *mm = h_sloballoc(mem, 1024);
    g_check_cmp_ptr(mm, !=, NULL);
    if (mm) {
        void *ptr = mm->alloc(mm, 100);
        g_check_cmp_ptr(ptr, !=, NULL);
    }
}

static void test_h_sloballoc_too_small(void) {
    uint8_t mem[10];
    HAllocator *mm = h_sloballoc(mem, 10);
    g_check_cmp_ptr(mm, ==, NULL);
}

static void test_sloballoc_zero(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 0);
    g_check_cmp_ptr(ptr, !=, NULL);
}

static void test_slob_realloc_shrink(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 200);

    void *mm = slobrealloc(slob, ptr, 100);
    g_check_cmp_ptr(mm, !=, NULL);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}

static void test_slob_realloc_expand(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 100);

    void *mm = slobrealloc(slob, ptr, 200);
    g_check_cmp_ptr(mm, !=, NULL);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}

static void test_slob_realloc_overflow(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 200);
    void *ptr = sloballoc(slob, 100);
    void *mm = slobrealloc(slob, ptr, 1024);
    g_check_cmp_ptr(mm, ==, NULL);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}
static void test_slob_realloc_zero_size(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 100);
    void *mm = slobrealloc(slob, ptr, 0);
    g_check_cmp_ptr(mm, ==, NULL);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}
static void test_slob_realloc_null_ptr(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *mm = slobrealloc(slob, NULL, 100);
    g_check_cmp_ptr(mm, !=, NULL);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}
static void test_slob_realloc_null(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    // it's performing sloballoc(slob, 0); which will allocate a slob of minimum size
    void *mm = slobrealloc(slob, NULL, 0);
    g_check_cmp_ptr(mm, !=, NULL);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}
static void test_slob_realloc_shrink_non_splittable(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);
    void *ptr = sloballoc(slob, 200);
    void *mm = slobrealloc(slob, ptr, 199);
    g_check_cmp_ptr(mm, !=, NULL);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}
static void test_slob_realloc_fallback(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);

    void *ptr = sloballoc(slob, 100);
    g_check_cmp_ptr(ptr, !=, NULL);
    memset(ptr, 0x33, 100);

    // Try to expand to a very large size so allocation should fail
    void *mm = slobrealloc(slob, ptr, 1000);
    g_check_cmp_ptr(mm, ==, NULL);

    // Original must still be valid and payload preserved
    for (size_t i = 0; i < 100; ++i)
        g_check_cmp_int(((unsigned char *)ptr)[i], ==, 0x33);

    g_check_cmp_int(slobcheck(slob), ==, 0);
}
static void test_slob_coalesce_after_shrink(void) {
    uint8_t mem[1024];
    SLOB *slob = slobinit(mem, 1024);

    void *a = sloballoc(slob, 200);
    void *b = sloballoc(slob, 200);
    slobfree(slob, a); // Free a block
    // Shrink b so its tail becomes a free block adjacent to a and should coalesce
    void *b2 = slobrealloc(slob, b, 100);
    g_check_cmp_int(slobcheck(slob), ==, 0);
}

void register_sloballoc_tests(void) {
    g_test_add_func("/core/sloballoc/slobinit", test_slobinit);
    g_test_add_func("/core/sloballoc/sloballoc_small_size", test_sloballoc_small_size);
    g_test_add_func("/core/sloballoc/sloballoc_overflow", test_sloballoc_overflow);
    g_test_add_func("/core/sloballoc/sloballoc_remblock", test_sloballoc_remblock);
    g_test_add_func("/core/sloballoc/sloballoc_exact_fit", test_sloballoc_exact_fit);
    g_test_add_func("/core/sloballoc/slobfree_left_adjacent", test_slobfree_left_adjacent);
    g_test_add_func("/core/sloballoc/slobfree_right_adjacent", test_slobfree_right_adjacent);
    g_test_add_func("/core/sloballoc/slobfree_both_adjacent", test_slobfree_both_adjacent);
    g_test_add_func("/core/sloballoc/slobfree_no_adjacent", test_slobfree_no_adjacent);
    g_test_add_func("/core/sloballoc/slobcheck", test_slobcheck);
    g_test_add_func("/core/sloballoc/h_sloballoc", test_h_sloballoc);
    g_test_add_func("/core/sloballoc/test_sloballoc_zero", test_sloballoc_zero);
    g_test_add_func("/core/sloballoc/h_sloballoc_too_small", test_h_sloballoc_too_small);
    g_test_add_func("/core/sloballoc/h_slob_realloc_shrink", test_slob_realloc_shrink);
    g_test_add_func("/core/sloballoc/h_slob_realloc_expand", test_slob_realloc_expand);
    g_test_add_func("/core/sloballoc/h_slob_realloc_overflow", test_slob_realloc_overflow);
    g_test_add_func("/core/sloballoc/test_slob_realloc_zero_size", test_slob_realloc_zero_size);
    g_test_add_func("/core/sloballoc/test_slob_realloc_null_ptr", test_slob_realloc_null_ptr);
    g_test_add_func("/core/sloballoc/test_slob_realloc_null", test_slob_realloc_null);
    g_test_add_func("/core/sloballoc/h_slob_realloc_shrink_non_splittable",
                    test_slob_realloc_shrink_non_splittable);
    g_test_add_func("/core/sloballoc/h_slob_realloc_fallback", test_slob_realloc_fallback);
    g_test_add_func("/core/sloballoc/test_slob_coalesce_after_shrink",
                    test_slob_coalesce_after_shrink);
}
