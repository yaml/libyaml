#include <stddef.h>
#include <stdint.h>

#include <yaml.h>

static int DiscardWriter(void *data, unsigned char *buffer, size_t size) {
    (void)data;
    (void)buffer;
    (void)size;
    return 1;  // pretend all output was written successfully
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    yaml_parser_t parser;
    yaml_emitter_t emitter;
    yaml_event_t event;

    if (!yaml_parser_initialize(&parser)) {
        return 0;
    }

    if (!yaml_emitter_initialize(&emitter)) {
        yaml_parser_delete(&parser);
        return 0;
    }

    yaml_parser_set_input_string(&parser, data, size);

    yaml_emitter_set_output(&emitter, DiscardWriter, nullptr);
    yaml_emitter_set_unicode(&emitter, 1);
    yaml_emitter_set_encoding(&emitter, YAML_UTF8_ENCODING);

    while (true) {
        if (!yaml_parser_parse(&parser, &event)) {
            break;
        }

        yaml_event_type_t type = event.type;

        if (!yaml_emitter_emit(&emitter, &event)) {
            break;
        }

        if (type == YAML_STREAM_END_EVENT) {
            break;
        }
    }

    yaml_emitter_delete(&emitter);
    yaml_parser_delete(&parser);
    return 0;
}
