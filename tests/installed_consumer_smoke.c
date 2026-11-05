#include <hammer/hammer.h>
#include <stdint.h>

int main(void) {
    const uint8_t input[] = {'a'};
    HParser *parser = h_ch('a');
    if (!parser)
        return 1;

    HParseResult *result = h_parse(parser, input, sizeof(input));
    if (!result) {
        h_parser_free(parser);
        return 1;
    }

    h_parse_result_free(result);
    h_parser_free(parser);
    return 0;
}
