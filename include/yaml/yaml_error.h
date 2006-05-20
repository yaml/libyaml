#ifndef YAML_ERROR_H
#define YAML_ERROR_H

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    YAML_NO_ERROR,

    YAML_MEMORY_ERROR,

    YAML_READER_ERROR,
    YAML_SCANNER_ERROR,
    YAML_PARSER_ERROR,

    YAML_WRITER_ERROR,
    YAML_EMITTER_ERROR
} yaml_error_type_t;

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_ERROR_H */
