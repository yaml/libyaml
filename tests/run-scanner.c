#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

int
main(int argc, char *argv[])
{
    int number;

    if (argc < 2) {
        printf("Usage: %s file1.yaml ...\n", argv[0]);
        return 0;
    }

    for (number = 1; number < argc; number ++)
    {
        FILE *file;
        yaml_parser_t *parser;
        yaml_token_t token;
        yaml_error_t error;
        char error_buffer[256];
        int done = 0;
        int count = 0;
        int failed = 0;

        printf("[%d] Scanning '%s': ", number, argv[number]);
        fflush(stdout);

        file = fopen(argv[number], "rb");
        assert(file);

        assert((parser = yaml_parser_new()));

        yaml_parser_set_file_reader(parser, file);

        while (!done)
        {
            if (!yaml_parser_parse_token(parser, &token)) {
                failed = 1;
                break;
            }

            done = (token.type == YAML_STREAM_END_TOKEN);

            yaml_token_destroy(&token);

            count ++;
        }

        yaml_parser_get_error(parser, &error);

        yaml_parser_delete(parser);

        assert(!fclose(file));

        yaml_error_message(&error, error_buffer, 256);

        printf("%s (%d tokens) -> %s\n",
                (failed ? "FAILURE" : "SUCCESS"),
                count, error_buffer);
    }

    return 0;
}

