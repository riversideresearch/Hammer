#include "../src/hammer.h"
#include "test_suite.h"

static void test_float(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_float32();
    const uint8_t input[4] = { 0x40, 0x49, 0x0F, 0xDB }; // pi
    HParseResult *result = h_parse(p, input, 4);
    float pi = result->ast->token_data.flt;
    fprintf(stderr, "pi is approx: %f\n", pi);
    g_check_parse_match(p, be,input, 4, "f0x1.921fb6p+1"); 
}

void register_floating_point_parser_tests(void) {
    g_test_add_data_func("/core/parser/float/float", GINT_TO_POINTER(PB_PACKRAT), test_float);
}
