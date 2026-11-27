#include <stddef.h>
#include <stdint.h>

#include <yaml.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    yaml_parser_t parser;
    yaml_token_t token;

    if (!yaml_parser_initialize(&parser)) {
        return 0;
    }

    yaml_parser_set_input_string(&parser, data, size);

    while (true) {
        if (!yaml_parser_scan(&parser, &token)) {
            break;
        }

        yaml_token_type_t type = token.type;
        yaml_token_delete(&token);

        if (type == YAML_STREAM_END_TOKEN) {
            break;
        }
    }

    yaml_parser_delete(&parser);
    return 0;
}
