#include <stddef.h>
#include <stdint.h>

#include <yaml.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    yaml_parser_t parser;
    yaml_event_t event;

    if (!yaml_parser_initialize(&parser)) {
        return 0;
    }

    yaml_parser_set_input_string(&parser, data, size);

    while (true) {
        if (!yaml_parser_parse(&parser, &event)) {
            break;
        }

        yaml_event_type_t type = event.type;
        yaml_event_delete(&event);

        if (type == YAML_STREAM_END_EVENT) {
            break;
        }
    }

    yaml_parser_delete(&parser);
    return 0;
}
