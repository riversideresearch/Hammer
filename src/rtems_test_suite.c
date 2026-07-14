/* Copyright (c) 2026 Riverside Research */

#include "rtems_test_suite.h"
#include <stdio.h>
#include "static_alloc.h"

HTestEntry h_test_registry[MAX_TESTS];
int h_test_count = 0;
int h_test_current_failed = 0;
int h_test_pass_count = 0;
int h_test_fail_count = 0;


void h_test_add(const char *name, void (*fn) (void)){
    if(h_test_count >= MAX_TESTS){
        printf(" [harness] ERROR: MAX_TESTS (%d) exceeded, could not register %s\n",
                MAX_TESTS, name);
        return;
    }
    h_test_registry[h_test_count].name = name;
    h_test_registry[h_test_count].fn = fn;
    h_test_count++;
}

void h_test_add_data(const char *name, void (*func)(const void*), const void *data) {
    if (h_test_count >= MAX_TESTS) return;
    h_test_registry[h_test_count].name = name;
    h_test_registry[h_test_count].fn = NULL;
    h_test_registry[h_test_count].data_func = func;
    h_test_registry[h_test_count].data = data;
    h_test_count++;
}

int h_test_run_all(void){
    printf("RTEMS HAMMER TEST SUITE\n");
    printf("Running %d tests\n\n", h_test_count);
    fflush(stdout);
    for(int i = 0; i<h_test_count; i++){
        printf("Test %d, name: %s\n", i+1, h_test_registry[i].name);
        if (h_test_registry[i].data_func) {
            h_test_registry[i].data_func(h_test_registry[i].data);
        } else {
            h_test_registry[i].fn();
        }
        printf("Successfully ran test function\n");
        if(h_test_current_failed){
            printf(" FAIL: %s\n", h_test_registry[i].name);
            h_test_fail_count++;
            h_test_current_failed = false;
        }else{
            printf(" PASS: %s\n", h_test_registry[i].name);
            h_test_pass_count++;
        }
        h_parse_pool_reset();
    }

    printf("\nRESULTS\n");
    printf("%d passed out of %d\n", h_test_pass_count, h_test_count);
    if(h_test_fail_count > 0){
        printf("%d failed out of %d\n", h_test_fail_count, h_test_count);
        return 1;
    }

    printf("All tests passed.\n");
    return 0;
}