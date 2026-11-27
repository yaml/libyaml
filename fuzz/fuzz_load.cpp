#include <stddef.h>
#include <stdint.h>

#include <yaml.h>

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
    yaml_parser_t parser;
    yaml_document_t document;

    if (!yaml_parser_initialize(&parser)) {
        return 0;
    }

    yaml_parser_set_input_string(&parser, data, size);

    if (yaml_parser_load(&parser, &document)) {
        yaml_document_delete(&document);
    }

    yaml_parser_delete(&parser);
    return 0;
}
