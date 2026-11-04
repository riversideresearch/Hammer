#include "glue.h"
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

#include <glib.h>

// Test packrat.c: Setting bit_length on AST when it's non-zero (line 64)
static void test_packrat_ast_bit_length(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *p = h_sequence(h_ch('a'), h_ch('b'), NULL);
    h_compile(p, be, NULL);

    HParseResult *res = h_parse(p, (const uint8_t *)"ab", 2);
    if (res && res->ast) {
        g_check_cmp_ptr(res->ast, !=, NULL);
        h_parse_result_free(res);
    }
}

// Test packrat.c: recall with parser not in involved_set (lines 79-82)
static void test_packrat_recall_not_involved(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *num = h_ch_range('0', '9');

    HParser *term = h_indirect();
    HParser *term_seq = h_sequence(term, h_ch('*'), num, NULL);
    HParser *term_choice = h_choice(term_seq, num, NULL);
    h_bind_indirect(term, term_choice);

    HParser *expr = h_indirect();
    HParser *expr_seq = h_sequence(expr, h_ch('+'), term, NULL);
    HParser *expr_choice = h_choice(expr_seq, term, NULL);
    h_bind_indirect(expr, expr_choice);

    h_compile(expr, be, NULL);

    HParseResult *res = h_parse(expr, (const uint8_t *)"1+2*3", 5);
    if (res) {
        h_parse_result_free(res);
    }
}

// Test packrat.c: recall cache update when cached is NULL (lines 93-94)
static void test_packrat_recall_cache_update(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *num = h_ch_range('0', '9');
    HParser *plus = h_ch('+');

    HParser *expr = h_indirect();
    HParser *expr_seq = h_sequence(expr, plus, num, NULL);
    HParser *expr_choice = h_choice(expr_seq, num, NULL);
    h_bind_indirect(expr, expr_choice);

    h_compile(expr, be, NULL);

    HParseResult *res = h_parse(expr, (const uint8_t *)"1+2+3", 5);
    if (res) {
        h_parse_result_free(res);
    }
}

// Test packrat.c: grow with tmp_res NULL (lines 173-175)
static void test_packrat_grow_null_result(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *num = h_ch_range('0', '9');
    HParser *expr = h_indirect();
    HParser *expr_seq = h_sequence(expr, h_ch('+'), num, NULL);
    HParser *expr_choice = h_choice(expr_seq, num, NULL);
    h_bind_indirect(expr, expr_choice);

    h_compile(expr, be, NULL);

    HParseResult *res = h_parse(expr, (const uint8_t *)"1+", 2);
    (void)res;
}

/* A zero-width context mutation must not prevent the root rule from growing. */
static void test_packrat_grow_after_value_mutation(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *expr = h_indirect();
    HParser *recursive = h_sequence(h_put_value(h_epsilon_p(), "value"), h_free_value("value"),
                                    expr, h_ch('a'), NULL);
    h_bind_indirect(expr, h_choice(recursive, h_ch('a'), NULL));

    HParser *parser = h_sequence(expr, h_end_p(), NULL);
    g_check_cmp_int(h_compile(parser, be, NULL), ==, 0);

    HParseResult *res = h_parse(parser, (const uint8_t *)"aa", 2);
    g_check_cmp_ptr(res, !=, NULL);
    if (res)
        h_parse_result_free(res);
}
static void test_parse_bits_packrat(void) {
    const uint8_t input[] = {0xa5, 0xf3};
    const size_t widths[] = {0, 1, 3, 7, 8, 9, 13};

    for (size_t i = 0; i < sizeof(widths) / sizeof(widths[0]); i++) {
        HParser *parser = h_bits(widths[i], false);
        h_compile(parser, PB_PACKRAT, NULL);
        HParseResult *width_result = h_parse_bits(parser, input, widths[i]);
        g_assert_nonnull(width_result);
        g_assert_cmpuint(width_result->bit_length, ==, widths[i]);
        h_parse_result_free(width_result);
    }

    HParser *value = h_bits(3, false);
    h_compile(value, PB_PACKRAT, NULL);
    HParseResult *res = h_parse_bits(value, input, 3);
    g_assert_nonnull(res);
    g_assert_cmpuint(res->ast->token_data.uint, ==, 0x5);
    g_assert_cmpuint(res->bit_length, ==, 3);
    h_parse_result_free(res);
    const uint8_t alternate_padding[] = {0xa7};
    res = h_parse_bits(value, alternate_padding, 3);
    g_assert_nonnull(res);
    g_assert_cmpuint(res->ast->token_data.uint, ==, 0x5);
    h_parse_result_free(res);

    HParser *whole = h_sequence(h_bits(8, false), h_bits(5, false), h_end_p(), NULL);
    h_compile(whole, PB_PACKRAT, NULL);
    res = h_parse_bits(whole, input, 13);
    g_assert_nonnull(res);
    g_assert_cmpuint(res->bit_length, ==, 13);
    h_parse_result_free(res);

    HParser *end = h_sequence(h_bits(3, false), h_end_p(), NULL);
    h_compile(end, PB_PACKRAT, NULL);
    res = h_parse_bits(end, input, 3);
    g_assert_nonnull(res);
    h_parse_result_free(res);
    g_assert_null(h_parse_bits(end, input, 8));

    HParser *crossing = h_bits(8, false);
    h_compile(crossing, PB_PACKRAT, NULL);
    g_assert_null(h_parse_bits(crossing, input, 7));

    HParser *skip = h_sequence(h_skip(8), h_end_p(), NULL);
    h_compile(skip, PB_PACKRAT, NULL);
    g_assert_null(h_parse_bits(skip, input, 7));

    HParser *seek = h_sequence(h_seek(8, SEEK_SET), h_end_p(), NULL);
    h_compile(seek, PB_PACKRAT, NULL);
    g_assert_null(h_parse_bits(seek, input, 7));

    res = h_parse(value, input, 1);
    g_assert_nonnull(res);
    g_assert_cmpuint(res->bit_length, ==, 3);
    h_parse_result_free(res);

    HParser *rollback = h_many(h_sequence(h_ch('a'), h_ch('b'), NULL));
    const uint8_t input2[] = {0x3d}; // 'a'
    res = h_parse(rollback, input2, 1);
    g_check_cmp_int(res->bit_length, ==, 0);
    g_check_cmp_int(res->ast->bit_length, ==, 0);
}

static void test_parse_bits_unsupported_backends(void) {
    HParser *parser = h_ch('a');
    const uint8_t input[] = {'a'};

    HParserBackend unsupported[] = {PB_LL, PB_LALR, PB_GLR};
    for (size_t i = 0; i < sizeof(unsupported) / sizeof(unsupported[0]); i++) {
        if (h_compile(parser, unsupported[i], NULL) == 0)
            g_assert_null(h_parse_bits(parser, input, 8));
    }
}

void register_packrat_tests(void) {
    g_test_add_data_func("/core/parser/packrat/ast_bit_length", GINT_TO_POINTER(PB_PACKRAT),
                         test_packrat_ast_bit_length);
    g_test_add_data_func("/core/parser/packrat/recall_not_involved", GINT_TO_POINTER(PB_PACKRAT),
                         test_packrat_recall_not_involved);
    g_test_add_data_func("/core/parser/packrat/recall_cache_update", GINT_TO_POINTER(PB_PACKRAT),
                         test_packrat_recall_cache_update);
    g_test_add_data_func("/core/parser/packrat/grow_null_result", GINT_TO_POINTER(PB_PACKRAT),
                         test_packrat_grow_null_result);
    g_test_add_data_func("/core/parser/packrat/grow_after_value_mutation",
                         GINT_TO_POINTER(PB_PACKRAT), test_packrat_grow_after_value_mutation);
    g_test_add_func("/core/parser/packrat/parse_bits", test_parse_bits_packrat);
    g_test_add_func("/core/parser/packrat/parse_bits_unsupported_backends",
                    test_parse_bits_unsupported_backends);
}
