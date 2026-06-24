/* Copyright (c) 2026 Riverside Research */
#include "cfgrammar.h"
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

#include <glib.h>

#define ITERATIONS 100
#define NUM_TYPES 5
#define BODY_SIZE 1

static HParser **build_body_parsers(void){
    HParser **bodies = malloc(NUM_TYPES * sizeof(HParser *));

    bodies[0] = h_ch(0x40);
    bodies[1] = h_ch(0x41);
    bodies[2] = h_ch(0x42);
    bodies[3] = h_ch(0x43);
    bodies[4] = h_ch(0x44);

    return bodies;
}

static void test_dispatch_basic_functionality(void) {

    uint8_t buf[256];
    int buf_size = 1 + BODY_SIZE;
    buf[0] = 1; // Opcode byte

    buf[1] = (uint8_t){0x41};

    HParser **bodies = build_body_parsers();

    HParser *discriminator = h_uint8();

    HParser *message = h_dispatch(discriminator, bodies, NUM_TYPES);

    HParseResult *res = h_parse(message, buf, buf_size);

    g_check_cmp_ptr(res, !=, NULL);
    g_check_cmp_ptr(res->ast, !=, NULL);
}

static void test_dispatch_incorrect_opcode(void) {

    uint8_t buf[256];
    int buf_size = 1 + BODY_SIZE;
    buf[0] = 2; // Incorrect opcode byte

    buf[1] = (uint8_t){0x41};

    HParser **bodies = HParser **bodies = build_body_parsers();

    HParser *discriminator = h_uint8();

    HParser *message = h_dispatch(discriminator, bodies, NUM_TYPES);

    HParseResult *res = h_parse(message, buf, buf_size);

    g_check_cmp_ptr(res, ==, NULL);
}

void register_dispatch_tests(void) {
    g_test_add_func("/core/dispatch/basic_functionality", test_dispatch_basic_functionality);
    g_test_add_func("/core/dispatch/incorrect_opcode", test_dispatch_incorrect_opcode);
}