#include "hammer.h"
#include "test_suite.h"

#include <glib.h>

static void test_regex_backend_registration(void) {
    g_check_cmp_int(h_is_backend_available(PB_REGULAR), ==, 1);
    g_check_string(h_get_name_for_backend(PB_REGULAR), ==, "regex");
    g_check_cmp_int(h_query_backend_by_name("regex"), ==, PB_REGULAR);
}

static void test_regex_basic_parsers(void) {
    g_check_parse_match(h_ch('a'), PB_REGULAR, "a", 1, "u0x61");
    g_check_parse_match(h_token((const uint8_t *)"abc", 3), PB_REGULAR, "abc", 3, "<61.62.63>");
    g_check_parse_match(h_choice(h_ch('a'), h_ch('b'), NULL), PB_REGULAR, "b", 1, "u0x62");
    g_check_parse_failed(h_ch('a'), PB_REGULAR, "b", 1);
}

static void test_regex_repetition_and_optional(void) {
    g_check_parse_match(h_many1(h_ch('a')), PB_REGULAR, "aaa", 3, "(u0x61 u0x61 u0x61)");
    g_check_parse_match(h_sequence(h_optional(h_ch('a')), h_ch('b'), NULL), PB_REGULAR, "b", 1,
                        "(null u0x62)");
    g_check_parse_match(h_sequence(h_optional(h_ignore(h_ch('a'))), h_ch('b'), NULL), PB_REGULAR,
                        "ab", 2, "(u0x62)");
    g_check_parse_match(h_sequence(h_optional(h_ignore(h_ch('a'))), h_ch('b'), NULL), PB_REGULAR,
                        "b", 1, "(null u0x62)");
}

static void test_regex_signed_integers(void) {
    g_check_parse_match(h_int8(), PB_REGULAR, "\xff", 1, "s-0x1");
    g_check_parse_match(h_int16(), PB_REGULAR, "\xff\xff", 2, "s-0x1");
    g_check_parse_match(h_int_range(h_int8(), -2, 0), PB_REGULAR, "\xff", 1, "s-0x1");
    g_check_parse_failed(h_int_range(h_int8(), -2, 0), PB_REGULAR, "\x7f", 1);
}

void register_regex_tests(void) {
    g_test_add_func("/core/backends/regex/registration", test_regex_backend_registration);
    g_test_add_func("/core/backends/regex/basic_parsers", test_regex_basic_parsers);
    g_test_add_func("/core/backends/regex/repetition_optional", test_regex_repetition_and_optional);
    g_test_add_func("/core/backends/regex/signed_integers", test_regex_signed_integers);
}
