/* Hosted-C99 core smoke test; deliberately has no GLib dependency. */
#include "hammer.h"
#include "platform.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int fail(const char *message) {
    fprintf(stderr, "portable smoke: %s\n", message);
    return 1;
}

int main(void) {
    static unsigned char input[] = {'x'};
    HParserGraph *graph;
    HParser *parser;
    HParseResult *result;
    HParserTestcase cases[2];
    HBenchmarkResults *benchmark;
    char *formatted = NULL;
    char *expected;
    struct HStopWatch stopwatch;

    if (h_platform_asprintf(&formatted, "%s %d", "hammer", 99) != 9 || formatted == NULL ||
        strcmp(formatted, "hammer 99") != 0) {
        free(formatted);
        return fail("formatting");
    }
    free(formatted);
    if (h_allocate_token_type("org.hammer.portable-smoke") < TT_USER)
        return fail("token registration");

    graph = h_parser_graph_begin();
    if (graph == NULL)
        return fail("graph begin");
    parser = h_ch('x');
    h_parser_graph_end(graph);

    result = h_parse(parser, input, sizeof(input));
    if (result == NULL) {
        h_parser_graph_free(graph);
        return fail("parse");
    }
    expected = h_write_result_unamb(result->ast);
    h_parse_result_free(result);
    if (expected == NULL) {
        h_parser_graph_free(graph);
        return fail("result formatting");
    }

    h_platform_stopwatch_reset(&stopwatch);
    if (h_platform_stopwatch_ns(&stopwatch) < 0) {
        h_parser_graph_free(graph);
        return fail("stopwatch");
    }

    cases[0].input = input;
    cases[0].length = sizeof(input);
    cases[0].output_unambiguous = expected;
    cases[1].input = NULL;
    cases[1].length = 0;
    cases[1].output_unambiguous = NULL;
    benchmark = h_benchmark(parser, cases);
    if (benchmark == NULL) {
        h_parser_graph_free(graph);
        return fail("benchmark");
    }
    h_parser_graph_free(graph);
    return 0;
}
