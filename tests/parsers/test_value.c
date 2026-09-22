#include "glue.h"
#include "hammer.h"
#include "internal.h"
#include "test_suite.h"

#include <glib.h>

// Test value.c: parse_put edge cases (line 26)
static void test_value_put_edge_cases(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *put_null_parser = h_put_value__m(&system_allocator, NULL, "test_key");
    h_compile(put_null_parser, be, NULL);
    HParseResult *res = h_parse(put_null_parser, (const uint8_t *)"abc", 3);
    g_check_cmp_ptr(res, ==, NULL);

    HParser *p = h_ch('a');
    HParser *put_null_key = h_put_value__m(&system_allocator, p, NULL);
    h_compile(put_null_key, be, NULL);
    HParseResult *res2 = h_parse(put_null_key, (const uint8_t *)"a", 1);
    g_check_cmp_ptr(res2, ==, NULL);

    HParser *put1 = h_put_value(p, "existing_key");
    HParser *put2 = h_put_value(p, "existing_key");
    HParser *seq = h_sequence(put1, put2, NULL);
    h_compile(seq, be, NULL);
    HParseResult *res3 = h_parse(seq, (const uint8_t *)"aa", 2);
    if (res3) {
        h_parse_result_free(res3);
    }
}

// Test value.c: parse_get edge cases (line 54)
static void test_value_get_edge_cases(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *get_null_key = h_get_value__m(&system_allocator, NULL);
    h_compile(get_null_key, be, NULL);
    HParseResult *res = h_parse(get_null_key, (const uint8_t *)"", 0);
    g_check_cmp_ptr(res, ==, NULL);
}

// Test value.c: parse_free edge cases (line 86)
static void test_value_free_edge_cases(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    HParser *free_null_key = h_free_value__m(&system_allocator, NULL);
    h_compile(free_null_key, be, NULL);
    HParseResult *res = h_parse(free_null_key, (const uint8_t *)"", 0);
    g_check_cmp_ptr(res, ==, NULL);
}

static void test_value_with_zero_width_prefix(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *expr = h_indirect();
    HParser *prefix =
        h_sequence(h_put_value(h_epsilon_p(), "value"), h_free_value("value"), expr, NULL);
    h_bind_indirect(expr, h_choice(prefix, h_ch('a'), NULL));
    HParser *parser = h_sequence(expr, h_end_p(), NULL);

    /*
     * The zero-width prefix changes the symbol generation before recursive
     * expr is evaluated at the same position. Packrat must use the active
     * seed despite the changed generation instead of recursing indefinitely.
     */
    g_check_parse_ok(parser, be, "a", 1);
}

static void test_value_string_keys_are_content_based_and_owned(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    char put_key[] = "hello";
    char get_key[] = "hello";
    HParser *get_put_child = h_ch('a');
    HParser *get_put = h_put_value(get_put_child, put_key);
    HParser *get = h_get_value(get_key);
    HParser *get_parser = h_sequence(get_put, get, NULL);
    // Independent mutations make borrowed names compare unequal.
    put_key[0] = 'x';
    get_key[0] = 'y';
    h_compile(get_parser, be, NULL);
    HParseResult *res = h_parse(get_parser, (const uint8_t *)"a", 1);
    g_check_cmp_ptr(res, !=, NULL);
    if (res)
        h_parse_result_free(res);
    h_parser_free(get_parser);
    h_parser_free(get);
    h_parser_free(get_put);
    h_parser_free(get_put_child);

    char free_put_key[] = "hello";
    char free_key[] = "hello";
    HParser *free_put_child = h_ch('a');
    HParser *free_put = h_put_value(free_put_child, free_put_key);
    HParser *free = h_free_value(free_key);
    HParser *free_parser = h_sequence(free_put, free, NULL);
    free_put_key[0] = 'x';
    free_key[0] = 'y';
    h_compile(free_parser, be, NULL);
    res = h_parse(free_parser, (const uint8_t *)"a", 1);
    g_check_cmp_ptr(res, !=, NULL);
    if (res)
        h_parse_result_free(res);
    h_parser_free(free_parser);
    h_parser_free(free);
    h_parser_free(free_put);
    h_parser_free(free_put_child);

    char remove_put_key[] = "hello";
    char remove_key[] = "hello";
    char after_remove_key[] = "hello";
    HParser *remove_put_child = h_ch('a');
    HParser *remove_put = h_put_value(remove_put_child, remove_put_key);
    HParser *remove = h_free_value(remove_key);
    HParser *after_remove = h_get_value(after_remove_key);
    HParser *free_then_get = h_sequence(remove_put, remove, after_remove, NULL);
    remove_put_key[0] = 'x';
    remove_key[0] = 'x';
    after_remove_key[0] = 'x';
    h_compile(free_then_get, be, NULL);
    res = h_parse(free_then_get, (const uint8_t *)"a", 1);
    g_check_cmp_ptr(res, ==, NULL);
    h_parser_free(free_then_get);
    h_parser_free(after_remove);
    h_parser_free(remove);
    h_parser_free(remove_put);
    h_parser_free(remove_put_child);
}

static void test_value_combinators_do_not_cache_or_leak_state(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    /*
     * h_get_value() is invoked twice at the same input position. The second
     * invocation must observe that h_free_value() removed the value rather
     * than returning the first invocation's cached success.
     */
    HParser *get = h_get_value("value");
    HParser *get_after_free =
        h_sequence(h_put_value(h_epsilon_p(), "value"), get, h_free_value("value"), get, NULL);
    g_check_parse_failed(get_after_free, be, "", 0);

    /*
     * A cached h_put_value() must still perform its insertion side effect
     * when it is invoked again at the same input position.
     */
    HParser *put = h_put_value(h_epsilon_p(), "value");
    HParser *put_after_free =
        h_sequence(put, h_free_value("value"), put, h_get_value("value"), NULL);
    g_check_parse_ok(put_after_free, be, "", 0);

    /*
     * A cached h_free_value() must still remove the current value, not just
     * return the result from the first removal.
     */
    HParser *free = h_free_value("value");
    HParser *free_after_reput =
        h_sequence(h_put_value(h_epsilon_p(), "value"), free, h_put_value(h_epsilon_p(), "value"),
                   free, h_get_value("value"), NULL);
    g_check_parse_failed(free_after_reput, be, "", 0);

    /*
     * Choice restores the input position between alternatives, but it must
     * also discard a value written by an alternative that subsequently fails.
     */
    HParser *failed_branch = h_sequence(h_put_value(h_epsilon_p(), "value"), h_nothing_p(), NULL);
    HParser *choice_leak = h_choice(failed_branch, h_get_value("value"), NULL);
    g_check_parse_failed(choice_leak, be, "", 0);

    /*
     * A failed branch that removes a value must restore it before the next
     * alternative is attempted.
     */
    HParser *failed_free_branch = h_sequence(h_free_value("value"), h_nothing_p(), NULL);
    HParser *choice_free_leak =
        h_sequence(h_put_value(h_epsilon_p(), "value"),
                   h_choice(failed_free_branch, h_get_value("value"), NULL), NULL);
    g_check_parse_ok(choice_free_leak, be, "", 0);
}

static void test_value_speculative_combinators(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);

    /*
     * The first permutation ordering stores "value" and then fails when its
     * second parser sees the duplicate name. The reverse ordering must be
     * able to succeed after the first ordering is abandoned.
     */
    HParser *p1 = h_choice(h_put_value(h_ch('b'), "value"),
                           h_sequence(h_get_value("value"), h_ch('a'), NULL), NULL);
    HParser *p2 = h_put_value(h_choice(h_ch('a'), h_ch('b'), NULL), "value");
    HParser *perm = h_permutation(p1, p2, NULL);
    g_check_parse_ok(perm, be, "ba", 2);

    /*
     * A separator can succeed before the following element fails. Its value
     * must not survive when h_sepBy restores the input to before the
     * incomplete iteration.
     */
    HParser *sep = h_put_value(h_ch(','), "separator");
    HParser *separated = h_sequence(h_sepBy(h_ch('a'), sep), h_get_value("separator"), NULL);
    g_check_parse_failed(separated, be, "a,x", 3);

    /*
     * Successful probes that do not consume input must not commit values.
     */
    HParser *and_probe =
        h_sequence(h_and(h_put_value(h_ch('a'), "and_probe")), h_get_value("and_probe"), NULL);
    g_check_parse_failed(and_probe, be, "a", 1);

    HParser *difference_probe = h_sequence(
        h_difference(h_token((const uint8_t *)"ab", 2), h_put_value(h_ch('a'), "difference_probe")),
        h_get_value("difference_probe"), NULL);
    g_check_parse_failed(difference_probe, be, "ab", 2);

    HParser *butnot_probe = h_sequence(
        h_butnot(h_token((const uint8_t *)"ab", 2), h_put_value(h_ch('a'), "butnot_probe")),
        h_get_value("butnot_probe"), NULL);
    g_check_parse_failed(butnot_probe, be, "ab", 2);

    /*
     * Relational probes and xor must evaluate p2 before p1's tentative
     * value changes become visible. The selected p1 result then commits.
     */
    HParser *butnot_transaction = h_sequence(
        h_butnot(h_put_value(h_ch('a'), "butnot_value"),
                 h_sequence(h_free_value("butnot_value"), h_ch('a'), NULL)),
        h_get_value("butnot_value"), NULL);
    g_check_parse_ok(butnot_transaction, be, "a", 1);

    HParser *difference_transaction = h_sequence(
        h_difference(h_put_value(h_ch('a'), "difference_value"),
                     h_sequence(h_free_value("difference_value"), h_ch('a'), NULL)),
        h_get_value("difference_value"), NULL);
    g_check_parse_ok(difference_transaction, be, "a", 1);

    HParser *xor_transaction =
        h_sequence(h_xor(h_put_value(h_ch('a'), "xor_value"), h_free_value("xor_value")),
                   h_get_value("xor_value"), NULL);
    g_check_parse_ok(xor_transaction, be, "a", 1);
}

static void test_value_left_recursion_generation(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *expr = h_indirect();
    HParser *tail = h_sequence(h_put_value(h_ch('a'), "value"), h_free_value("value"), NULL);

    h_bind_indirect(expr, h_choice(h_sequence(expr, tail, NULL), h_ch('a'), NULL));
    HParser *complete = h_sequence(expr, h_end_p(), NULL);

    /*
     * Each tail changes the symbol-table generation twice. The active
     * left-recursion seed must still be available to the next growth pass.
     */
    g_check_parse_ok(complete, be, "aa", 2);
    g_check_parse_ok(complete, be, "aaa", 3);
}

static void test_value_store_free_before_recursive_call(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *expr = h_indirect();
    HParser *tail = h_sequence(h_put_value(h_ch('a'), "value"), h_free_value("value"), NULL);

    h_bind_indirect(expr, h_choice(h_sequence(tail, expr, NULL), h_ch('a'), NULL));
    HParser *complete = h_sequence(expr, h_end_p(), NULL);

    /*
     * The value is stored and freed before the recursive call. This ordering
     * must not invalidate the recursive parse state or its cached result.
     */
    g_check_parse_ok(complete, be, "aa", 2);
    g_check_parse_ok(complete, be, "aaa", 3);
}

static void test_value_left_recursion_discards_non_growth(gconstpointer backend) {
    HParserBackend be = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *expr = h_indirect();
    HParser *non_growing = h_put_value(h_epsilon_p(), "value");

    h_bind_indirect(expr, h_choice(h_sequence(expr, non_growing, NULL), h_ch('a'), NULL));
    HParser *complete = h_sequence(expr, h_not(h_get_value("value")), h_end_p(), NULL);

    /*
     * The recursive alternative succeeds but consumes no more input. Its
     * insertion is abandoned with the candidate and must not leak.
     */
    g_check_parse_ok(complete, be, "a", 1);
}

void register_value_tests(void) {
    g_test_add_data_func("/core/parser/packrat/value_put_edge_cases", GINT_TO_POINTER(PB_PACKRAT),
                         test_value_put_edge_cases);
    g_test_add_data_func("/core/parser/packrat/value_get_edge_cases", GINT_TO_POINTER(PB_PACKRAT),
                         test_value_get_edge_cases);
    g_test_add_data_func("/core/parser/packrat/value_free_edge_cases", GINT_TO_POINTER(PB_PACKRAT),
                         test_value_free_edge_cases);
    g_test_add_data_func("/core/parser/packrat/value_with_zero_width_prefix", GINT_TO_POINTER(PB_PACKRAT),
                         test_value_with_zero_width_prefix);
    g_test_add_data_func("/core/parser/packrat/value_string_keys_are_content_based_and_owned",
                         GINT_TO_POINTER(PB_PACKRAT),
                         test_value_string_keys_are_content_based_and_owned);
    g_test_add_data_func("/core/parser/packrat/value_combinators_do_not_cache_or_leak_state",
                         GINT_TO_POINTER(PB_PACKRAT),
                         test_value_combinators_do_not_cache_or_leak_state);
    g_test_add_data_func("/core/parser/packrat/value_speculative_combinators",
                         GINT_TO_POINTER(PB_PACKRAT), test_value_speculative_combinators);
    g_test_add_data_func("/core/parser/packrat/value_left_recursion_generation",
                         GINT_TO_POINTER(PB_PACKRAT), test_value_left_recursion_generation);
    g_test_add_data_func("/core/parser/packrat/value_store_free_before_recursive_call",
                         GINT_TO_POINTER(PB_PACKRAT), test_value_store_free_before_recursive_call);
    g_test_add_data_func("/core/parser/packrat/value_left_recursion_discards_non_growth",
                         GINT_TO_POINTER(PB_PACKRAT),
                         test_value_left_recursion_discards_non_growth);
}
