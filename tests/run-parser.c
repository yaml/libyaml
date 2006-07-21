#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int
main(int argc, char *argv[])
{
    FILE *file;
    yaml_parser_t parser;
    yaml_event_t event;
    int done = 0;
    int count = 0;

    if (argc != 2) {
        printf("Usage: %s file.yaml\n", argv[0]);
        return 0;
    }
    file = fopen(argv[1], "rb");
    assert(file);

    assert(yaml_parser_initialize(&parser));

    yaml_parser_set_input_file(&parser, file);

    while (!done)
    {
        assert(yaml_parser_parse(&parser, &event));

        done = (event.type == YAML_STREAM_END_EVENT);

        yaml_event_delete(&event);

        count ++;
    }

    yaml_parser_delete(&parser);

    fclose(file);

    printf("Parsing the file '%s': %d events\n", argv[1], count);

    return 0;
}

