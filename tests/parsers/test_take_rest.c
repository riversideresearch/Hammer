/* Copyright (c) 2026 Riverside Research */
#include "hammer.h"
#include "test_suite.h"

#include <glib.h>

typedef struct {
    unsigned int calls;
    const uint8_t *bytes;
    size_t length;
} TakeRestActionState;

static HParsedToken *record_take_rest_action(const HParseResult *result, void *user_data) {
    TakeRestActionState *state = user_data;

    g_assert_nonnull(result);
    g_assert_nonnull(result->ast);
    g_assert_cmpint(result->ast->token_type, ==, TT_BYTES);

    state->calls++;
    state->bytes = result->ast->token_data.bytes.token;
    state->length = result->ast->token_data.bytes.len;
    return (HParsedToken *)result->ast;
}

static void test_take_rest(gconstpointer backend) {
    HParserBackend selected = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *parser = h_take_rest();

    g_check_parse_match(parser, selected, "", 0, "<>");
    g_check_parse_match(parser, selected, "\x00\x7f\x80\xff", 4, "<00.7f.80.ff>");
}

static void test_take_rest_zero_copy(gconstpointer backend) {
    const uint8_t input[] = {0x00, 0x7f, 0x80, 0xff};
    HParserBackend selected = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *parser = h_take_rest();
    HParseResult *result;

    g_assert_cmpint(h_compile(parser, selected, NULL), ==, 0);
    result = h_parse(parser, input, sizeof(input));

    g_assert_nonnull(result);
    g_assert_nonnull(result->ast);
    g_assert_cmpint(result->ast->token_type, ==, TT_BYTES);
    g_assert_cmpuint(result->ast->token_data.bytes.len, ==, sizeof(input));
    g_assert_true(result->ast->token_data.bytes.token == input);
    h_parse_result_free(result);
}

static void test_take_rest_after_prefix(gconstpointer backend) {
    const uint8_t input[] = {0x5a, 0x00, 0x7f, 0x80, 0xff};
    HParserBackend selected = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *parser = h_sequence(h_ch(0x5a), h_take_rest(), h_end_p(), NULL);
    HParseResult *result;
    const HCountedArray *sequence;
    const HParsedToken *suffix;

    g_assert_cmpint(h_compile(parser, selected, NULL), ==, 0);
    result = h_parse(parser, input, sizeof(input));

    g_assert_nonnull(result);
    g_assert_nonnull(result->ast);
    g_assert_cmpint(result->ast->token_type, ==, TT_SEQUENCE);
    sequence = result->ast->token_data.seq;
    g_assert_nonnull(sequence);
    g_assert_cmpuint(sequence->used, ==, 2);
    suffix = sequence->elements[1];
    g_assert_cmpint(suffix->token_type, ==, TT_BYTES);
    g_assert_cmpuint(suffix->token_data.bytes.len, ==, sizeof(input) - 1);
    g_assert_true(suffix->token_data.bytes.token == input + 1);
    h_parse_result_free(result);
}

static void test_take_rest_action_once(gconstpointer backend) {
    const uint8_t input[] = {0x12, 0x34, 0x56, 0x78};
    HParserBackend selected = (HParserBackend)GPOINTER_TO_INT(backend);
    TakeRestActionState state = {0};
    HParser *parser = h_action(h_take_rest(), record_take_rest_action, &state);
    HParseResult *result;

    g_assert_cmpint(h_compile(parser, selected, NULL), ==, 0);
    result = h_parse(parser, input, sizeof(input));

    g_assert_nonnull(result);
    g_assert_cmpuint(state.calls, ==, 1);
    g_assert_true(state.bytes == input);
    g_assert_cmpuint(state.length, ==, sizeof(input));
    h_parse_result_free(result);
}

static void test_take_rest_rejects_bit_offset(void) {
    const uint8_t input[] = {0x80};
    HParser *parser = h_sequence(h_bits(1, false), h_take_rest(), NULL);

    g_check_parse_failed(parser, PB_PACKRAT, input, sizeof(input));
}

void register_take_rest_tests(void) {
    g_test_add_data_func("/core/parser/packrat/take_rest", GINT_TO_POINTER(PB_PACKRAT),
                         test_take_rest);
    g_test_add_data_func("/core/parser/packrat/take_rest_zero_copy", GINT_TO_POINTER(PB_PACKRAT),
                         test_take_rest_zero_copy);
    g_test_add_data_func("/core/parser/packrat/take_rest_after_prefix", GINT_TO_POINTER(PB_PACKRAT),
                         test_take_rest_after_prefix);
    g_test_add_data_func("/core/parser/packrat/take_rest_action_once", GINT_TO_POINTER(PB_PACKRAT),
                         test_take_rest_action_once);
    g_test_add_func("/core/parser/packrat/take_rest_rejects_bit_offset",
                    test_take_rest_rejects_bit_offset);
    g_test_add_data_func("/core/backends/regex/take_rest", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest);
    g_test_add_data_func("/core/backends/regex/take_rest_zero_copy", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_zero_copy);
    g_test_add_data_func("/core/backends/regex/take_rest_after_prefix", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_after_prefix);
    g_test_add_data_func("/core/backends/regex/take_rest_action_once", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_action_once);
}
