/**
 * @file yaml.h
 * @brief Public interface for libyaml.
 * 
 * Include the header file with
 * @code
 * #include <yaml/yaml.h>
 * @endcode
 */

#ifndef YAML_H
#define YAML_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

#include "yaml_version.h"
#include "yaml_error.h"

typedef enum {
    YAML_DETECT_ENCODING,
    YAML_UTF8_ENCODING,
    YAML_UTF16LE_ENCODING,
    YAML_UTF16BE_ENCODING
} yaml_encoding_t;

typedef enum {
    YAML_ANY_SCALAR_STYLE,
    YAML_PLAIN_SCALAR_STYLE,
    YAML_SINGLE_QUOTED_SCALAR_STYLE,
    YAML_DOUBLE_QUOTED_SCALAR_STYLE,
    YAML_LITERAL_SCALAR_STYLE,
    YAML_FOLDED_SCALAR_STYLE
} yaml_scalar_style_t;

typedef enum {
    YAML_ANY_SEQUENCE_STYLE,
    YAML_BLOCK_SEQUENCE_STYLE,
    YAML_FLOW_SEQUENCE_STYLE
} yaml_sequence_style_t;

typedef enum {
    YAML_ANY_MAPPING_STYLE,
    YAML_BLOCK_MAPPING_STYLE,
    YAML_FLOW_MAPPING_STYLE
} yaml_mapping_style_t;

typedef enum {
    YAML_STREAM_START_TOKEN,
    YAML_STREAM_END_TOKEN,

    YAML_VERSION_DIRECTIVE_TOKEN,
    YAML_TAG_DIRECTIVE_TOKEN,
    YAML_DOCUMENT_START_TOKEN,
    YAML_DOCUMENT_END_TOKEN,

    YAML_BLOCK_SEQUENCE_START_TOKEN,
    YAML_BLOCK_MAPPING_START_TOKEN,
    YAML_BLOCK_END_TOKEN,

    YAML_FLOW_SEQUENCE_START_TOKEN,
    YAML_FLOW_SEQUENCE_END_TOKEN,
    YAML_FLOW_MAPPING_START_TOKEN,
    YAML_FLOW_MAPPING_END_TOKEN,

    YAML_BLOCK_ENTRY_TOKEN,
    YAML_FLOW_ENTRY_TOKEN,
    YAML_KEY_TOKEN,
    YAML_VALUE_TOKEN,

    YAML_ALIAS_TOKEN,
    YAML_ANCHOR_TOKEN,
    YAML_TAG_TOKEN,
    YAML_SCALAR_TOKEN
} yaml_token_type_t;

typedef enum {
    YAML_STREAM_START_EVENT,
    YAML_STREAM_END_EVENT,

    YAML_DOCUMENT_START_EVENT,
    YAML_DOCUMENT_END_EVENT,

    YAML_ALIAS_EVENT,
    YAML_SCALAR_EVENT,

    YAML_SEQUENCE_START_EVENT,
    YAML_SEQUENCE_END_EVENT,

    YAML_MAPPING_START_EVENT,
    YAML_MAPPING_END_EVENT
} yaml_event_type_t;

typedef struct {
    size_t offset;
    size_t index;
    size_t line;
    size_t column;
} yaml_mark_t;

typedef struct {
    yaml_error_type_t type;
    char *context;
    yaml_mark_t context_mark;
    char *problem;
    yaml_mark_t problem_mark;
} yaml_error_t;

typedef struct {
    yaml_token_type_t type;
    union {
        yaml_encoding_t encoding;
        char *anchor;
        char *tag;
        struct {
            char *value;
            size_t length;
            yaml_scalar_style_t style;
        } scalar;
        struct {
            int major;
            int minor;
        } version;
        struct {
          char *handle;
          char *prefix;
        } tag_pair;
    } data;
    yaml_mark_t start_mark;
    yaml_mark_t end_mark;
} yaml_token_t;

typedef struct {
    yaml_event_type_t type;
    union {
        struct {
            yaml_encoding_t encoding;
        } stream_start;
        struct {
            struct {
                int major;
                int minor;
            } version;
            struct {
                char *handle;
                char *prefix;
            } **tag_pairs;
            int implicit;
        } document_start;
        struct {
            int implicit;
        } document_end;
        struct {
            char *anchor;
        } alias;
        struct {
            char *anchor;
            char *tag;
            char *value;
            size_t length;
            int plain_implicit;
            int quoted_implicit;
            yaml_scalar_style_t style;
        } scalar;
        struct {
            char *anchor;
            char *tag;
            int implicit;
            yaml_sequence_style_t style;
        } sequence_start;
        struct {
            char *anchor;
            char *tag;
            int implicit;
            yaml_mapping_style_t style;
        } mapping_start;
    } data;
    yaml_mark_t start_mark;
    yaml_mark_t end_mark;
} yaml_event_t;

/*
typedef struct {
} yaml_parser_t;

typedef struct {
} yaml_emitter_t;
*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_H */

