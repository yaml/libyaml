#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

/*
 * Test that the scanner enforces the maximum nesting depth.
 */

static int
scan_string(const char *input, size_t length)
{
    yaml_parser_t parser;
    yaml_token_t token;
    int done = 0;
    int error = 0;

    assert(yaml_parser_initialize(&parser));
    yaml_parser_set_input_string(&parser,
            (const unsigned char *)input, length);

    while (!done) {
        if (!yaml_parser_scan(&parser, &token)) {
            error = 1;
            break;
        }
        done = (token.type == YAML_STREAM_END_TOKEN);
        yaml_token_delete(&token);
    }

    yaml_parser_delete(&parser);
    return error;
}

/*
 * Parse the input into events.  Return 1 on a parse error, 0 otherwise, and
 * store the deepest collection nesting seen in *max_depth.
 */

static int
parse_string(const char *input, size_t length, int *max_depth)
{
    yaml_parser_t parser;
    yaml_event_t event;
    int done = 0;
    int error = 0;
    int depth = 0;

    *max_depth = 0;
    assert(yaml_parser_initialize(&parser));
    yaml_parser_set_input_string(&parser,
            (const unsigned char *)input, length);

    while (!done) {
        if (!yaml_parser_parse(&parser, &event)) {
            error = 1;
            break;
        }
        if (event.type == YAML_SEQUENCE_START_EVENT
                || event.type == YAML_MAPPING_START_EVENT) {
            depth++;
            if (depth > *max_depth)
                *max_depth = depth;
        }
        else if (event.type == YAML_SEQUENCE_END_EVENT
                || event.type == YAML_MAPPING_END_EVENT) {
            depth--;
        }
        done = (event.type == YAML_STREAM_END_EVENT);
        yaml_event_delete(&event);
    }

    yaml_parser_delete(&parser);
    return error;
}

/*
 * Build "a:\n- k:\n  - k:\n    - k:\n..." with the given number of indentless
 * sequences, each holding a block mapping: 2 * levels + 1 nested collections,
 * while the scanner only sees levels + 1 indentation levels.
 */

static char *
indentless_input(int levels, size_t *length)
{
    size_t len = 3 + 1;
    size_t pos = 0;
    char *input;
    int i, j;

    for (i = 0; i < levels; i++)
        len += 2 * i + 5;
    len += 2 * levels + 5;
    input = (char *)malloc(len);
    assert(input);

    memcpy(input + pos, "a:\n", 3);
    pos += 3;
    for (i = 0; i < levels; i++) {
        for (j = 0; j < 2 * i; j++)
            input[pos++] = ' ';
        memcpy(input + pos, "- k:\n", 5);
        pos += 5;
    }
    for (j = 0; j < 2 * levels; j++)
        input[pos++] = ' ';
    memcpy(input + pos, "v: 1\n", 5);
    pos += 5;
    input[pos] = '\0';

    *length = pos;
    return input;
}

/*
 * Build "[a: [a: [a: ... x]]]" with the given number of flow sequences, each
 * holding a single-pair mapping: 2 * levels nested collections, while the
 * scanner only sees levels flow levels.
 */

static char *
flow_pair_input(int levels, size_t *length)
{
    size_t len = (size_t)levels * 5 + 2;
    size_t pos = 0;
    char *input;
    int i;

    input = (char *)malloc(len);
    assert(input);

    for (i = 0; i < levels; i++) {
        memcpy(input + pos, "[a: ", 4);
        pos += 4;
    }
    input[pos++] = 'x';
    for (i = 0; i < levels; i++)
        input[pos++] = ']';
    input[pos] = '\0';

    *length = pos;
    return input;
}

int
main(void)
{
    char *input;
    size_t length;
    int max_depth;
    int i;

    /* Test 1: nesting beyond the default limit (1000) must fail. */
    {
        int depth = 2000;
        input = (char *)malloc(depth + 1);
        assert(input);
        memset(input, '[', depth);
        input[depth] = '\0';

        printf("Test 1: %d nested '[' (exceeds limit) ... ", depth);
        fflush(stdout);
        assert(scan_string(input, depth) == 1);
        printf("OK (rejected)\n");
        free(input);
    }

    /* Test 2: block nesting beyond the limit must fail. */
    {
        /*
         * Build a YAML string with 2000 levels of block mapping nesting:
         * "a:\n b:\n  c:\n   d:\n..." — each level indented one more space.
         */
        int depth = 2000;
        int len = 0;
        int pos = 0;

        /* Each level: indent spaces + "X:\n" */
        for (i = 0; i < depth; i++)
            len += i + 2 + 1;  /* i spaces + "X:" + newline */
        input = (char *)malloc(len + 1);
        assert(input);

        for (i = 0; i < depth; i++) {
            int j;
            for (j = 0; j < i; j++)
                input[pos++] = ' ';
            input[pos++] = 'a' + (i % 26);
            input[pos++] = ':';
            input[pos++] = '\n';
        }
        input[pos] = '\0';

        printf("Test 2: %d levels block nesting (exceeds limit) ... ", depth);
        fflush(stdout);
        assert(scan_string(input, pos) == 1);
        printf("OK (rejected)\n");
        free(input);
    }

    /* Test 3: flow nesting within the limit must succeed. */
    {
        int depth = 500;
        int len = depth * 2;
        input = (char *)malloc(len + 1);
        assert(input);
        for (i = 0; i < depth; i++)
            input[i] = '[';
        for (i = 0; i < depth; i++)
            input[depth + i] = ']';
        input[len] = '\0';

        printf("Test 2: %d nested '[]' (within limit) ... ", depth);
        fflush(stdout);
        assert(scan_string(input, len) == 0);
        printf("OK (accepted)\n");
        free(input);
    }

    /*
     * Test 4: indentless sequences add a nesting level that the scanner does
     * not count, so the parser has to enforce the limit.
     */
    {
        input = indentless_input(600, &length);
        printf("Test 4: 600 indentless sequence levels (exceeds limit) ... ");
        fflush(stdout);
        assert(parse_string(input, length, &max_depth) == 1);
        assert(max_depth <= 1000);
        printf("OK (rejected)\n");
        free(input);

        input = indentless_input(400, &length);
        printf("Test 4: 400 indentless sequence levels (within limit) ... ");
        fflush(stdout);
        assert(parse_string(input, length, &max_depth) == 0);
        assert(max_depth == 801);
        printf("OK (accepted)\n");
        free(input);
    }

    /*
     * Test 5: single-pair mappings in flow sequences add a nesting level that
     * the scanner does not count either.
     */
    {
        input = flow_pair_input(600, &length);
        printf("Test 5: 600 flow sequences of single-pair mappings (exceeds limit) ... ");
        fflush(stdout);
        assert(parse_string(input, length, &max_depth) == 1);
        assert(max_depth <= 1000);
        printf("OK (rejected)\n");
        free(input);

        input = flow_pair_input(400, &length);
        printf("Test 5: 400 flow sequences of single-pair mappings (within limit) ... ");
        fflush(stdout);
        assert(parse_string(input, length, &max_depth) == 0);
        assert(max_depth == 800);
        printf("OK (accepted)\n");
        free(input);
    }

    return 0;
}
