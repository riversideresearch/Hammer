#include "../src/hammer.h"
#include "test_suite.h"

static void test_float16(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_float16();
    const uint8_t input[2] = { 0x03, 0xFF}; // pi
    HParseResult *result = h_parse(p, input, 2);
    float pi = result->ast->token_data.flt;
    fprintf(stderr, "result: %f\n", pi);
    g_check_parse_match(p, be, input, 2, "..."); 
}

static void test_float32(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_float32();
    const uint8_t input[4] = { 0x40, 0x49, 0x0F, 0xDB }; // 3.1415927
    HParseResult *result = h_parse(p, input, 4);
    float pi = result->ast->token_data.flt;
    fprintf(stderr, "pi is approx: %f\n", pi);
    g_check_parse_match(p, be, input, 4, "f0x1.921fb6p+1"); 
}

static void test_double64(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_double64();
    const uint8_t input[8] = { 0x40, 0x49, 0x0F, 0xDB, }; 
    HParseResult *result = h_parse(p, input, 8);
    float pi = result->ast->token_data.flt;
    fprintf(stderr, "result: %f\n", pi);
    g_check_parse_match(p, be, input, 8, "..."); 
}

static void test_decimal_float(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_decimal_float();
    const uint8_t input[4] = { 0x40, 0x49, 0x0F, 0xDB }; // 3.1415927
    HParseResult *result = h_parse(p, input, 4);
    float pi = result->ast->token_data.flt;
    fprintf(stderr, "pi is approx: %f\n", pi);
    g_check_parse_match(p, be, input, 4, "f0x1.921fb6p+1"); 
}

static void test_decimal_double(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_decimal_double();
    const uint8_t input[8] = { 0x40, 0x49, 0x0F, 0xDB, }; 
    HParseResult *result = h_parse(p, input, 8);
    float pi = result->ast->token_data.flt;
    fprintf(stderr, "result: %f\n", pi);
    g_check_parse_match(p, be, input, 8, "..."); 
}


void register_floating_point_parser_tests(void) {
    g_test_add_data_func("/core/parser/float/float32", GINT_TO_POINTER(PB_PACKRAT), test_float32);
}
