#if defined(RTEMS_BUILD) && defined(STATIC_BUILD)
#include "static_alloc.h"
#include "test_suite.h"

#include <stdint.h>

static void test_static_alloc_pool_accounting(void) {
    h_static_alloc_init();

    void *grammar = h_grammar_allocator->alloc(h_grammar_allocator, 1);
    void *parse = h_parse_allocator->alloc(h_parse_allocator, 1);

    g_assert_nonnull(grammar);
    g_assert_nonnull(parse);
    g_assert_cmpuint((uintptr_t)grammar % 8u, ==, 0);
    g_assert_cmpuint((uintptr_t)parse % 8u, ==, 0);
    g_assert_cmpuint(h_grammar_pool_used(), ==, 8);
    g_assert_cmpuint(h_parse_pool_used(), ==, 8);
    g_assert_cmpuint(h_parse_pool_high_water(), ==, 8);

    h_parse_pool_reset();
    g_assert_cmpuint(h_parse_pool_used(), ==, 0);
    g_assert_cmpuint(h_parse_pool_high_water(), ==, 8);
}

static void test_static_alloc_parse_bits_uses_parse_pool(void) {
    const uint8_t input[] = {0xa0};

    h_static_alloc_init();
    h_default_allocator = h_grammar_allocator;
    h_default_parse_allocator = h_parse_allocator;

    HParser *parser = h_bits(3, false);
    g_assert_nonnull(parser);
    g_assert_cmpint(h_compile(parser, PB_PACKRAT, NULL), ==, 0);
    g_assert_cmpuint(h_parse_pool_used(), ==, 0);

    HParseResult *result = h_parse_bits(parser, input, 3);
    g_assert_nonnull(result);
    g_assert_cmpuint(result->ast->token_data.uint, ==, 5);
    g_assert_cmpuint(h_parse_pool_used(), >, 0);
    h_parse_result_free(result);

    h_parse_pool_reset();
    g_assert_cmpuint(h_parse_pool_used(), ==, 0);

    result = h_parse_bits_debug(parser, input, 3, NULL, false);
    g_assert_nonnull(result);
    g_assert_cmpuint(result->ast->token_data.uint, ==, 5);
    g_assert_cmpuint(h_parse_pool_used(), >, 0);
    h_parse_result_free(result);

    h_parse_pool_reset();
    g_assert_cmpuint(h_parse_pool_used(), ==, 0);
}

void register_static_alloc_tests(void) {
    g_test_add_func("/core/static_alloc/pool_accounting", test_static_alloc_pool_accounting);
    g_test_add_func("/core/static_alloc/parse_bits_uses_parse_pool",
                    test_static_alloc_parse_bits_uses_parse_pool);
}
#endif /* defined(RTEMS_BUILD) && defined(STATIC_BUILD) */
