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

int
main(void)
{
    char *input;
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

    /* Test 2: nesting within the limit must succeed. */
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

    return 0;
}
