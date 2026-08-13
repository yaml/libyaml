#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

/*
 * Regression test for a leading BOM being counted as a column when the input
 * encoding is set explicitly (https://github.com/yaml/libyaml/issues/334).
 *
 * With an explicit encoding the reader does not strip the BOM, so the scanner
 * skips it instead. That skip must not advance the column: otherwise the first
 * token starts at column 1 and a root-level block mapping fails to parse with
 * "did not find expected <document start>".
 */

static int
parse_scalars(const char *input, size_t length, yaml_encoding_t encoding)
{
    yaml_parser_t parser;
    yaml_event_t event;
    int scalars = 0;
    int done = 0;

    assert(yaml_parser_initialize(&parser));
    yaml_parser_set_encoding(&parser, encoding);
    yaml_parser_set_input_string(&parser, (const unsigned char *)input, length);

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            scalars = -1;
            break;
        }
        if (event.type == YAML_SCALAR_EVENT)
            scalars++;
        done = (event.type == YAML_STREAM_END_EVENT);
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    return scalars;
}

int
main(void)
{
    /* UTF-8 BOM followed by a root-level block mapping with two entries. */
    const char input[] = "\xEF\xBB\xBF" "a: b\nc: d\n";
    size_t length = sizeof(input) - 1;

    printf("BOM + explicit UTF-8 encoding ... ");
    fflush(stdout);
    assert(parse_scalars(input, length, YAML_UTF8_ENCODING) == 4);
    printf("OK\n");

    printf("BOM + detected encoding ... ");
    fflush(stdout);
    assert(parse_scalars(input, length, YAML_ANY_ENCODING) == 4);
    printf("OK\n");

    return 0;
}
