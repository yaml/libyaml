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
    int idx;

    if (argc < 2) {
        printf("Usage: %s file1.yaml ...\n", argv[0]);
        return 0;
    }

    for (idx = 1; idx < argc; idx ++)
    {
        FILE *file;
        yaml_parser_t *parser;
        yaml_event_t event;
        int failed = 0;
        int count = 0;

        printf("[%d] Parsing '%s': ", idx, argv[idx]);
        fflush(stdout);

        file = fopen(argv[idx], "rb");
        assert(file);

        parser = yaml_parser_new();
        assert(parser);

        yaml_parser_set_file_reader(parser, file);

        while (1)
        {
            if (!yaml_parser_parse_event(parser, &event)) {
                failed = 1;
                break;
            }

            if (event.type == YAML_NO_EVENT)
                break;

            yaml_event_destroy(&event);

            count ++;
        }

        if (!failed) {
            printf("SUCCESS (%d tokens)\n", count);
        }
        else {
            yaml_error_t error;
            char message[256];
            yaml_parser_get_error(parser, &error);
            yaml_error_message(&error, message, 256);
            printf("FAILURE (%d events)\n -> %s\n", count, message);
        }

        yaml_parser_delete(parser);

        fclose(file);
    }

    return 0;
}

