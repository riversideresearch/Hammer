#include "hammer.h"
#include "test_suite.h"

#include <glib.h>

static HParser *make_ab_or_ac(void) {
    HParser *ab = h_sequence(h_ch('a'), h_ch('b'), NULL);
    HParser *ac = h_sequence(h_ch('a'), h_ch('c'), NULL);
    return h_choice(ab, ac, NULL);
}

static void test_contextfree_backend_registration(void) {
    g_check_cmp_int(h_is_backend_available(PB_LL), ==, 1);
    g_check_cmp_int(h_is_backend_available(PB_LALR), ==, 1);
    g_check_cmp_int(h_is_backend_available(PB_GLR), ==, 1);

    g_check_string(h_get_name_for_backend(PB_LL), ==, "llk");
    g_check_string(h_get_name_for_backend(PB_LALR), ==, "lalr");
    g_check_string(h_get_name_for_backend(PB_GLR), ==, "glr");

    g_check_cmp_int(h_query_backend_by_name("llk"), ==, PB_LL);
    g_check_cmp_int(h_query_backend_by_name("lalr"), ==, PB_LALR);
    g_check_cmp_int(h_query_backend_by_name("glr"), ==, PB_GLR);
}

static void test_llk_requires_enough_lookahead(void) {
    HParser *p = make_ab_or_ac();

    g_check_cmp_int(h_compile(p, PB_LL, NULL), ==, -2);
    g_check_cmp_int(h_compile(p, PB_LL, (void *)2), ==, 0);

    HParseResult *res = h_parse(p, (const uint8_t *)"ac", 2);
    g_check_cmp_ptr(res, !=, NULL);
    h_parse_result_free(res);
}

static void test_llk_backend_params_by_name(void) {
    HParserBackendWithParams *be = h_get_backend_with_params_by_name("llk(2)");
    g_check_cmp_ptr(be, !=, NULL);
    g_check_cmp_int(be->backend, ==, PB_LL);
    g_check_inttype("%zu", size_t, (size_t)(uintptr_t)be->params, ==, 2);

    HParser *p = make_ab_or_ac();
    g_check_cmp_int(h_compile_for_backend_with_params(p, be), ==, 0);

    HParseResult *res = h_parse(p, (const uint8_t *)"ab", 2);
    g_check_cmp_ptr(res, !=, NULL);
    h_parse_result_free(res);
    h_free_backend_with_params(be);
}

static void test_lalr_parse_simple_contextfree_grammar(void) {
    HParser *p = h_sequence(h_ch('a'), h_choice(h_ch('b'), h_ch('c'), NULL), h_ch('d'), NULL);

    g_check_cmp_int(h_compile(p, PB_LALR, NULL), ==, 0);

    HParseResult *res = h_parse(p, (const uint8_t *)"acd", 3);
    g_check_cmp_ptr(res, !=, NULL);
    h_parse_result_free(res);
}

static void test_glr_accepts_lalr_conflict(void) {
    HParser *d = h_ch('d');
    HParser *expr = h_indirect();
    HParser *sum = h_sequence(expr, h_ch('+'), expr, NULL);
    h_bind_indirect(expr, h_choice(sum, d, NULL));

    g_check_cmp_int(h_compile(expr, PB_GLR, NULL), ==, 0);

    HParseResult *res = h_parse(expr, (const uint8_t *)"d+d+d", 5);
    g_check_cmp_ptr(res, !=, NULL);
    h_parse_result_free(res);
}

void register_contextfree_backend_tests(void) {
    g_test_add_func("/core/backends/contextfree/registration",
                    test_contextfree_backend_registration);
    g_test_add_func("/core/backends/contextfree/llk_lookahead", test_llk_requires_enough_lookahead);
    g_test_add_func("/core/backends/contextfree/llk_params_by_name",
                    test_llk_backend_params_by_name);
    g_test_add_func("/core/backends/contextfree/lalr_parse",
                    test_lalr_parse_simple_contextfree_grammar);
    g_test_add_func("/core/backends/contextfree/glr_conflict", test_glr_accepts_lalr_conflict);
}
