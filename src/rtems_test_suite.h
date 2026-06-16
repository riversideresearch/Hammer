/* Copyright (c) 2026 Riverside Research */
/*
 * Minimal test harness for RTEMS
 * Provides same macro names as glib to make integration with existing testing easier
 */

#ifndef RTEMS_TEST_SUITE_H
#define RTEMS_TEST_SUITE_H

#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

#include "hammer.h"
#include "internal.h"

/*
 * Test Registry
 */
#define MAX_TESTS 512

/** 
 * @struct HTestEntry
 * @brief Hold one registered test
 * @name Path string passed to add test function
 * @fn function pointer
 */
typedef struct {
    const char *name;
    void (*fn)(void);
    const void *data;
    void (*data_func)(const void*);
} HTestEntry;

extern HTestEntry h_test_registry[MAX_TESTS];
extern int h_test_count;
extern int h_test_current_failed;
extern int h_test_pass_count;
extern int h_test_fail_count;

/**
 * test registration and execution
 */
void h_test_add(const char *name, void (*fn)(void));
int h_test_run_all(void);

/**
 * @brief Registers a test function under a path-style name
 */
#define g_test_add_func(name, fn) do { h_test_add((name), (fn)); } while(0)

#define g_test_add_data_func(name, data, func) h_test_add_data((name), (void(*)(const void*))(func), (const void*)(data))

void h_test_add_data(const char *name, void (*func)(const void*), const void *data);

#define g_test_init(argc, argv, ...) do { (void)(argc); (void)(argv); } while(0)
#define g_run_test() h_test_run_all()

/**
 * @brief Used when test runner invoked in slow mode. No such concept on rtems
 */
#define g_test_slow() 0
/**
 * @brief Used when test runner invoked in performance mode. No such concept on rtems
 */
#define g_test_perf() 0


/*
 * Failure reporting
 */
/**
 * @brief Marks current test as failed
 */
#define g_test_fail() do { h_test_current_failed = 1; } while(0)

/**
 * @brief Print a diagnostic message
 */
#define g_test_message(fmt, ...) do { printf(" [msg] " fmt "\n", ##__VA_ARGS__); } while(0)

/**
 * @brief ... nah
 */
#define g_test_verbose() 0

/*
 * Comparison assertions
 */

 /**
  * 
  */
#define g_check_inttype(fmt, typ, n1, op, n2) \
    do { \
        typ _n1 = (typ)(n1); \
        typ _n2 = (typ)(n2);  \
        if (!(_n1 op _n2)) { \
            printf("  [FAIL] %s:%d Check failed: (%s %s %s): " \
                   "(" fmt " %s " fmt ")\n", \
                   __FILE__, __LINE__, \
                   #n1, #op, #n2, \
                   _n1, #op, _n2); \
            fflush(stdout); \
            g_test_fail(); \
        } \
    } while(0)

#define g_check_cmp_int(n1, op, n2)    g_check_inttype("%d", int, n1, op, n2)
#define g_check_cmp_int32(n1, op, n2)  g_check_inttype("%d", int32_t, n1, op, n2)
#define g_check_cmp_int64(n1, op, n2)  g_check_inttype("%" PRId64, int64_t,  n1, op, n2)
#define g_check_cmp_uint32(n1, op, n2) g_check_inttype("%u", uint32_t, n1, op, n2)
#define g_check_cmp_uint64(n1, op, n2) g_check_inttype("%" PRIu64, uint64_t, n1, op, n2)
#define g_check_cmp_size(n1, op, n2)   g_check_inttype("%zu", size_t, n1, op, n2)
#define g_check_cmpfloat(n1, op, n2)   g_check_inttype("%g", float, n1, op, n2)
#define g_check_cmpdouble(n1, op, n2)  g_check_inttype("%g", double, n1, op, n2)

#define g_assert_true(expr) if(!expr) g_test_fail();

/**
 * pointer and byte comparison
 */
#define g_check_cmp_ptr(p1, op, p2) \
    do{ \
        const void *_p1 = (p1); \
        const void *_p2 = (p2); \
        if (!(_p1 op _p2)) { \
            printf(" [FAIL] %s:%d Check failed: (%s %s %s): " \
                   "(%p %s %p)\n", \
                   __FILE__, __LINE__, \
                   #p1, #op, #p2, _p1, #op, _p2); \
            fflush(stdout); \
            g_test_fail();   \
        } \
    } while (0)

#define g_check_bytes(len, n1, op, n2) \
    do { \
        if(!(memcmp((n1), (n2), (len)) op 0)) { \
            printf("  [FAIL] %s:%d Byte check failed: %s %s %s\n", \
                   __FILE__, __LINE__, #n1, #op, #n2); \
            fflush(stdout); \
            g_test_fail(); \
        } \
    }while(0)

/**
 * string comparison
 */
#define g_check_string(n1, op, n2) \
    do { \
        const char *_s1 = (n1); \
        const char *_s2 = (n2); \
        if(!(strcmp(_s1, _s2) op 0)){ \
            printf(" [FAIL] %s:%d Byte check failed: %s %s %s\n", \
                    __FILE__, __LINE__, #n1, #op, #n2);\
            fflush(stdout); \
            g_test_fail(); \
        } \
    }while(0)

#define g_check_maybe_string_eq(n1, op, n2) \
    do { \
        const char *_s1 = (n1); \
        const char *_s2 = (n2); \
        if(_s1 && _s2){ \
            g_check_string(_s1, ==, _s2); \
        } else if(_s1 != _s2){ \
            printf(" [FAIL] %s:%d NULL mismatch: %s == %s\n", \
                    __FILE__, __LINE__, #n1, #n2 ); \
            fflush(stdout); \
            g_test_fail(); \
        } \
    }while(0)

#define g_assert_cmpstr(n1, op, n2) \
    do { \
        const char *_s1 = (n1); \
        const char *_s2 = (n2); \
        if(_s1 && _s2){ \
            g_check_string(_s1, ==, _s2); \
        } else if(_s1 != _s2){ \
            printf(" [FAIL] %s:%d NULL mismatch: %s == %s\n", \
                    __FILE__, __LINE__, #n1, #n2); \
            fflush(stdout); \
            g_test_fail(); \
        } \
    }while(0)

/**
 * Parse assertions
 */
#define g_check_parse_ok(parser, backend, input, len) \
    do { \
        if (h_compile((parser), (backend), NULL)){ \
            printf(" [FAIL] %s:%d Compile failed\n", \
                    __FILE__, __LINE__); \
            fflush(stdout); \
            g_test_fail(); \
            break; \
        } \
        HParseResult *_r = h_parse((parser), (const uint8_t*)(input), (len)); \
        if(!_r){\
            printf(" [FAIL] %s:%d Parse failed",\
                    __FILE__, __LINE__); \
            fflush(stdout); \
            g_test_fail(); \
        } else { \
            h_parse_result_free(_r); \
        }\
    }while(0)

#define g_check_parse_failed(parser, backend, input, len) \
    do { \
        if (h_compile((parser), (backend), NULL) != 0){ \
            printf(" [FAIL] %s:%d Compile failed\n", \
                    __FILE__, __LINE__); \
            fflush(stdout); \
            g_test_fail(); \
            break; \
        } \
        HParseResult *_r = h_parse((parser), (const uint8_t*)(input), (len)); \
        if(_r){\
            printf(" [FAIL] %s:%d Parse succeeded (should fail)", \
                    __FILE__, __LINE__); \
            fflush(stdout); \
            g_test_fail(); \
            h_parse_result_free(_r); \
        } \
    } while(0)

#define g_check_parse_match(parser, backend, input, len, expected) \
    do { \
        if (h_compile((parser), (backend), NULL) != 0){ \
            printf(" [FAIL] %s:%d Compile failed\n", \
                    __FILE__, __LINE__); \
            fflush(stdout); \
            g_test_fail(); \
            break; \
        } \
        HParseResult *_r = h_parse((parser), (uint8_t*)(input), (len)); \
        if(!_r){ \
            printf(" [FAIL] %s:%d Parse failed\n", \
                    __FILE__, __LINE__); \
            fflush(stdout); \
            g_test_fail(); \
        } else { \
            char *_s = h_write_result_unamb(_r); \
            g_check_string(_s, ==, (expected)); \
            (&system_allocator)->free(&system_allocator, _s); \
            h_parse_result_free(_r); \
        } \
    } while(0)

#endif