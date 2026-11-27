#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

/*
 * Test: %TAG directive inheritance across documents in a multi-document stream.
 *
 * YAML 1.1 spec section 7.4:
 *   "If the document specifies no directives, it is parsed using the same
 *    settings as the previous document. If the document does specify any
 *    directives, all directives of previous documents, if any, are ignored."
 *
 * libyaml bug: yaml_parser_parse_document_end() unconditionally clears
 * parser->tag_directives, so document 2 (which has no directives) cannot
 * inherit the %TAG mapping from document 1.  When the parser then encounters
 * !x!bar in document 2 it cannot resolve the handle and throws a parse
 * error. The assert inside the loop will FAIL until the bug is fixed.
 */

static const unsigned char yaml_input[] =
    "%YAML 1.1\n"
    "%TAG !x! tag:example.com,2026:\n"
    "--- !x!food\n"
    "x: 0\n"
    "--- !x!bar\n"
    "x: 1\n";

int main(void)
{
    yaml_parser_t parser;
    yaml_event_t event;
    int done = 0;

    assert(yaml_parser_initialize(&parser));
    yaml_parser_set_input_string(&parser, yaml_input, sizeof(yaml_input) - 1);

    printf("Testing Directive Inheritance ... ");
    fflush(stdout);

    while (!done) {
        /* With the bug, this assert fires when the parser hits !x!bar
         * in doc 2 and cannot resolve the handle.  A spec-compliant
         * parser must succeed for the entire stream. */
        assert(yaml_parser_parse(&parser, &event));
        done = (event.type == YAML_STREAM_END_EVENT);
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    printf("PASSED\n");
    return 0;
}