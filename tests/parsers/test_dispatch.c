/* Copyright (c) 2026 Riverside Research */
#include "cfgrammar.h"
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

#include <glib.h>

#define ITERATIONS 100
#define NUM_TYPES 14
#define BODY_SIZE 1

static void test_dispatch_basic_functionality(void) {

    uint8_t buf[256];
    int buf_size = 1 + BODY_SIZE;
    buf[0] = 1; // Opcode byte

    buf[1] = (uint8_t){0x41};

    OpcodeMap entries[] = {{1, h_ch(0x41)}, {2, h_ch(0x42)}, {2013, h_uint32()}};
    HParser *discriminator = h_uint8();

    HParser *message = h_dispatch(discriminator, entries);

    HParseResult *res = h_parse(message, buf, buf_size);

    g_check_cmp_ptr(res, !=, NULL);
    g_check_cmp_ptr(res->ast, !=, NULL);
}

static void test_dispatch_incorrect_opcode(void) {

    uint8_t buf[256];
    int buf_size = 1 + BODY_SIZE;
    buf[0] = 2; // Incorrect opcode byte

    buf[1] = (uint8_t){0x41};

    OpcodeMap entries[] = {{1, h_ch(0x41)}, {2, h_ch(0x42)}, {2013, h_uint32()}};

    HParser *discriminator = h_uint8();

    HParser *message = h_dispatch(discriminator, entries);

    HParseResult *res = h_parse(message, buf, buf_size);

    g_check_cmp_ptr(res, ==, NULL);
}

static void test_dispatch_32_bits(void) {

    uint8_t buf[256];
    int buf_size = 8;

    // opcode = 2013 (0x000007DD)
    buf[0] = 0x00;
    buf[1] = 0x00;
    buf[2] = 0x07;
    buf[3] = 0xDD;

    // body = 2048 (0x00000800)
    buf[4] = 0x00;
    buf[5] = 0x00;
    buf[6] = 0x08;
    buf[7] = 0x00;

    OpcodeMap entries[] = {{1, h_ch(0x41)}, {2, h_ch(0x42)}, {2013, h_uint32()}};

    HParser *message = h_dispatch(h_uint32(), entries);

    HParseResult *res = h_parse(message, buf, buf_size);
}

void register_dispatch_tests(void) {
    g_test_add_func("/core/dispatch/basic_functionality", test_dispatch_basic_functionality);
    g_test_add_func("/core/dispatch/incorrect_opcode", test_dispatch_incorrect_opcode);
    g_test_add_func("/core/dispatch/32_bits", test_dispatch_32_bits);
}