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

#ifndef RTEMS_BUILD
typedef struct {
    const HParser *parser;
    const uint8_t *input;
    size_t length;
    bool passed;
} TakeRestThreadCase;

static gpointer parse_take_rest_concurrently(gpointer user_data) {
    TakeRestThreadCase *test = user_data;

    test->passed = true;
    for (size_t i = 0; i < 200; i++) {
        HParseResult *result = h_parse(test->parser, test->input, test->length);

        if (!result || !result->ast || result->ast->token_type != TT_BYTES ||
            result->ast->token_data.bytes.token != test->input ||
            result->ast->token_data.bytes.len != test->length) {
            test->passed = false;
        }
        if (result)
            h_parse_result_free(result);
        if (!test->passed)
            break;
    }
    return NULL;
}

static void test_take_rest_thread_safe(gconstpointer backend) {
    const uint8_t left_input[] = {0x00, 0x7f, 0x80, 0xff};
    const uint8_t right_input[] = {0x5a, 0xa5, 0x01};
    HParserBackend selected = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *parser = h_take_rest();
    TakeRestThreadCase left = {parser, left_input, sizeof(left_input), false};
    TakeRestThreadCase right = {parser, right_input, sizeof(right_input), false};
    GThread *left_thread;
    GThread *right_thread;

    g_assert_cmpint(h_compile(parser, selected, NULL), ==, 0);
    left_thread = g_thread_new("take-rest-left", parse_take_rest_concurrently, &left);
    right_thread = g_thread_new("take-rest-right", parse_take_rest_concurrently, &right);
    g_thread_join(left_thread);
    g_thread_join(right_thread);

    g_assert_true(left.passed);
    g_assert_true(right.passed);
}
#endif

typedef struct {
    const HParser *parser;
    const uint8_t *input;
    size_t length;
    unsigned int calls;
    bool nested_parse_succeeded;
} TakeRestReentrantState;

static HParsedToken *parse_take_rest_reentrantly(const HParseResult *result, void *user_data) {
    TakeRestReentrantState *state = user_data;
    HParseResult *nested;

    state->calls++;
    nested = h_parse(state->parser, state->input, state->length);
    state->nested_parse_succeeded = nested && nested->ast && nested->ast->token_type == TT_BYTES &&
                                    nested->ast->token_data.bytes.token == state->input &&
                                    nested->ast->token_data.bytes.len == state->length;
    if (nested)
        h_parse_result_free(nested);
    return (HParsedToken *)result->ast;
}

static void test_take_rest_reentrant(gconstpointer backend) {
    const uint8_t outer_input[] = {0x12, 0x34};
    const uint8_t nested_input[] = {0xab, 0xcd, 0xef};
    HParserBackend selected = (HParserBackend)GPOINTER_TO_INT(backend);
    HParser *take_rest = h_take_rest();
    TakeRestReentrantState state = {take_rest, nested_input, sizeof(nested_input), 0, false};
    HParser *parser = h_action(take_rest, parse_take_rest_reentrantly, &state);
    HParseResult *result;

    g_assert_cmpint(h_compile(take_rest, selected, NULL), ==, 0);
    g_assert_cmpint(h_compile(parser, selected, NULL), ==, 0);
    result = h_parse(parser, outer_input, sizeof(outer_input));

    g_assert_nonnull(result);
    g_assert_cmpuint(state.calls, ==, 1);
    g_assert_true(state.nested_parse_succeeded);
    h_parse_result_free(result);
}

static void test_take_rest_waits_for_final_chunk(void) {
    const uint8_t input[] = {0x12, 0x34};
    HParser *parser = h_take_rest();
    HSuspendedParser *suspended;
    HParseResult *result;

    g_assert_cmpint(h_compile(parser, PB_PACKRAT, NULL), ==, 0);
    suspended = h_parse_start(parser);
    g_assert_nonnull(suspended);
    g_assert_true(!h_parse_chunk(suspended, input, sizeof(input)));

    result = h_parse_finish(suspended);
    g_assert_nonnull(result);
    g_assert_nonnull(result->ast);
    g_assert_cmpint(result->ast->token_type, ==, TT_BYTES);
    g_assert_cmpuint(result->ast->token_data.bytes.len, ==, sizeof(input));
    g_check_bytes(result->ast->token_data.bytes.len, result->ast->token_data.bytes.token, ==, input);
    h_parse_result_free(result);
}

static void test_take_rest_rejects_partial_byte(void) {
    const uint8_t input[] = {0x80};
    HParser *bit_offset = h_sequence(h_bits(1, false), h_take_rest(), NULL);
    HParser *margin =
        h_sequence(h_bits(1, false),
                   h_with_endianness(BYTE_BIG_ENDIAN | BIT_LITTLE_ENDIAN, h_take_rest()), NULL);
    HParser *bit_limited = h_take_rest();

    g_check_parse_failed(bit_offset, PB_PACKRAT, input, sizeof(input));
    g_check_parse_failed(margin, PB_PACKRAT, input, sizeof(input));
    g_assert_cmpint(h_compile(bit_limited, PB_PACKRAT, NULL), ==, 0);
    g_assert_null(h_parse_bits(bit_limited, input, 1));
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
#ifndef RTEMS_BUILD
    g_test_add_data_func("/core/parser/packrat/take_rest_thread_safe", GINT_TO_POINTER(PB_PACKRAT),
                         test_take_rest_thread_safe);
#endif
    g_test_add_data_func("/core/parser/packrat/take_rest_reentrant", GINT_TO_POINTER(PB_PACKRAT),
                         test_take_rest_reentrant);
#if !defined(RTEMS_BUILD) || !defined(STATIC_BUILD)
    g_test_add_func("/core/parser/packrat/take_rest_waits_for_final_chunk",
                    test_take_rest_waits_for_final_chunk);
#endif
    g_test_add_func("/core/parser/packrat/take_rest_rejects_partial_byte",
                    test_take_rest_rejects_partial_byte);
    g_test_add_data_func("/core/backends/regex/take_rest", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest);
    g_test_add_data_func("/core/backends/regex/take_rest_zero_copy", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_zero_copy);
    g_test_add_data_func("/core/backends/regex/take_rest_after_prefix", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_after_prefix);
    g_test_add_data_func("/core/backends/regex/take_rest_action_once", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_action_once);
#ifndef RTEMS_BUILD
    g_test_add_data_func("/core/backends/regex/take_rest_thread_safe", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_thread_safe);
#endif
    g_test_add_data_func("/core/backends/regex/take_rest_reentrant", GINT_TO_POINTER(PB_REGULAR),
                         test_take_rest_reentrant);
}
