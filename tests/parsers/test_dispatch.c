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
    buf[0] = 0; // Incorrect opcode byte

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

    g_check_cmp_ptr(res, !=, NULL);
    g_check_cmp_ptr(res->ast, !=, NULL);
}

static void test_dispatch_bit_length_sequence(void) {
    /* Build a buffer containing three messages back‑to‑back */
    uint8_t buf[256];

    /* Each message is: opcode byte + one body byte */
    buf[0] = 1;
    buf[1] = 0x41; /* message 1 */
    buf[2] = 2;
    buf[3] = 0x42; /* message 2 */
    buf[4] = 3;
    buf[5] = 0x43; /* message 3 */

    size_t buf_size = 6;

    /* Three dispatch tables, one for each message */
    OpcodeMap entries1[] = {{1, h_ch(0x41)}};
    OpcodeMap entries2[] = {{2, h_ch(0x42)}};
    OpcodeMap entries3[] = {{3, h_ch(0x43)}};

    /* Each dispatch parses one opcode + one body byte */
    HParser *d1 = h_dispatch(h_uint8(), entries1);
    HParser *d2 = h_dispatch(h_uint8(), entries2);
    HParser *d3 = h_dispatch(h_uint8(), entries3);

    /* Sequence of three dispatches */
    HParser *seq = h_sequence(d1, d2, d3, NULL);

    /* Parse the whole buffer */
    HParseResult *res = h_parse(seq, buf, buf_size);

    g_check_cmp_ptr(res, !=, NULL);
    g_check_cmp_ptr(res->ast, !=, NULL);

    /*
     * Expected bit_length:
     * Each dispatch consumes 2 bytes = 16 bits.
     * Three dispatches = 48 bits.
     */
    size_t expected_bits = 3 * 16;

    g_check_cmp_int(res->bit_length, ==, expected_bits);
}

void register_dispatch_tests(void) {
    g_test_add_func("/core/dispatch/basic_functionality", test_dispatch_basic_functionality);
    g_test_add_func("/core/dispatch/incorrect_opcode", test_dispatch_incorrect_opcode);
    g_test_add_func("/core/dispatch/32_bits", test_dispatch_32_bits);
    g_test_add_func("/core/dispatch/bit_length", test_dispatch_bit_length_sequence);
}