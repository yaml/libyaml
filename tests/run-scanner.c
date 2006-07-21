#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

int
main(int argc, char *argv[])
{
    FILE *file;
    yaml_parser_t parser;
    yaml_token_t token;
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
        assert(yaml_parser_scan(&parser, &token));

        done = (token.type == YAML_STREAM_END_TOKEN);

        yaml_token_delete(&token);

        count ++;
    }

    yaml_parser_delete(&parser);

    fclose(file);

    printf("Parsing the file '%s': %d tokens\n", argv[1], count);

    return 0;
}

