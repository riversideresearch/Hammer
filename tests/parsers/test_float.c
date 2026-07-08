#include "hammer.h"
#include "test_suite.h"

#include <glib.h>
#include <string.h>

static void test_float(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    const HParser *p = h_float32();
    char input[4] = { 0x40, 0x49, 0x0F, 0xDB };
    g_check_parse_match(p, be,input, 4, "f0x1.921fb6p+1"); // pi
}

void register_floating_point_parser_tests(void) {
    g_test_add_data_func("/core/parser/float/float", GINT_TO_POINTER(PB_PACKRAT), test_float);
}
