/*****************************************************************************
 * Public Interface for LibYAML
 *
 * Copyright (c) 2006 Kirill Simonov
 *
 * LibYAML is free software; you can use, modify and/or redistribute it under
 * the terms of the MIT license; see the file LICENCE for more details.
 *****************************************************************************/

/*
 * General guidelines.
 *
 * Naming conventions: all functions exported by LibYAML starts with the `yaml_` prefix;
 * types starts with `yaml_` and ends with `_t`; macros and enumerations starts
 * with `YAML_`.
 *
 * FIXME: Calling conventions.
 * FIXME: Memory allocation.
 * FIXME: Errors and exceptions.
 * FIXME: And so on, and so forth.
 */


#ifndef YAML_H
#define YAML_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/*****************************************************************************
 * Export Definitions
 *****************************************************************************/

/*
 * Public API declarations.
 *
 * The following definitions are relevant only for the Win32 platform.  If you
 * are building LibYAML as a static library or linking your application against
 * LibYAML compiled as a static library, define the macro
 * `YAML_DECLARE_STATIC`.  If you are building LibYAML as a dynamic library
 * (DLL), you need to define `YAML_DECLARE_EXPORT`.  You don't need to define
 * any macros in case you are linking your application against LibYAML compiled
 * as DLL.
 *
 * There is no need to define any macros if you use LibYAML on non-Win32
 * platform.
 */

#ifdef WIN32
#   if defined(YAML_DECLARE_STATIC)
#       define  YAML_DECLARE(type)  type
#   elif defined(YAML_DECLARE_EXPORT)
#       define  YAML_DECLARE(type)  __declspec(dllexport) type
#   else
#       define  YAML_DECLARE(type)  __declspec(dllimport) type
#   endif
#else
#   define  YAML_DECLARE(type)  type
#endif

/*****************************************************************************
 * Version Information
 *****************************************************************************/

/*
 * The major, minor and patch version numbers of LibYAML.
 */

#define YAML_VERSION_MAJOR  0
#define YAML_VERSION_MINOR  2
#define YAML_VERSION_PATCH  0

/*
 * The version of LibYAML as a string.
 */

#define YAML_VERSION_STRING "0.2.0"

/*
 * Get the library version numbers at runtime.
 *
 * Arguments:
 *
 * - `major`: a pointer to store the major version number.
 *
 * - `minor`: a pointer to store the minor version number.
 *
 * - `patch`: a pointer to store the patch version number.
 */

YAML_DECLARE(void)
yaml_get_version(int *major, int *minor, int *patch);

/*
 * Get the library version as a string at runtime.
 *
 * Returns: the version of LibYAML as a static string.
 */

YAML_DECLARE(const char *)
yaml_get_version_string(void);

/*****************************************************************************
 * Error Handling
 *****************************************************************************/

/*
 * The type of the error.
 *
 * The YAML parser and emitter reports any error details using the
 * `yaml_error_t` structure.  The error type shows what subsystem generated the
 * error and what additional information about the error is available.
 */

typedef enum yaml_error_type_e {
    /* No error was produced. */
    YAML_NO_ERROR,

    /* Cannot allocate or reallocate a block of memory. */
    YAML_MEMORY_ERROR,

    /* Cannot read from the input stream. */
    YAML_READER_ERROR,
    /* Cannot decode a character in the input stream. */
    YAML_DECODER_ERROR,
    /* Cannot scan a YAML token. */
    YAML_SCANNER_ERROR,
    /* Cannot parse a YAML event. */
    YAML_PARSER_ERROR,
    /* Cannot compose a YAML document. */
    YAML_COMPOSER_ERROR,

    /* Cannot write into the output stream. */
    YAML_WRITER_ERROR,
    /* Cannot emit a YAML event. */
    YAML_EMITTER_ERROR,
    /* Cannot serialize a YAML document. */
    YAML_SERIALIZER_ERROR,

    /* Cannot resolve an implicit YAML node tag. */
    YAML_RESOLVER_ERROR
} yaml_error_type_t;

/*
 * Position in the input stream.
 *
 * Marks are used to indicate the position of YAML tokens, events and documents
 * in the input stream as well as to point to the place where a parser error has
 * occured.
 */

typedef struct yaml_mark_s {
    /* The number of the character in the input stream (starting from zero). */
    size_t index;
    /* The line in the input stream (starting from zero). */
    size_t line;
    /* The column in the input stream (starting from zero). */
    size_t column;
} yaml_mark_t;

/*
 * Error details.
 *
 * The structure gives detailed information on any problem that occured during
 * parsing or emitting.
 */

typedef struct yaml_error_s {

    /* The error type. */
    yaml_error_type_t type;

    /* The specific information for each error type. */
    union {

        /*
         * A problem occured while reading the input stream (relevant for
         * `YAML_READER_ERROR` and `YAML_DECODER_ERROR`).
         */
        struct {
            /* The problem description. */
            const char *problem;
            /* The position in the input stream, in bytes. */
            size_t offset;
            /* The problematic octet or character (`-1` if not applicable). */
            int value;
        } reading;

        /*
         * A problem occured while loading YAML data from the input stream
         * (relevant for `YAML_SCANNER_ERROR`, `YAML_PARSER_ERROR`, and
         * `YAML_COMPOSER_ERROR`).
         */
        struct {
            /* The description of the context in which the problem occured
               (`NULL` if not applicable). */
            const char *context;
            /* The context mark (if `context` is not `NULL`). */
            yaml_mark_t context_mark;
            /* The problem description. */
            const char *problem;
            /* The problem mark. */
            yaml_mark_t problem_mark;
        } loading;

        /*
         * A problem occured while writing into the output stream (relevant for
         * `YAML_WRITER_ERROR`).
         */
        struct {
            /* The problem description. */
            const char *problem;
            /* The position in the output stream, in bytes. */
            size_t offset;
        } writing;

        /*
         * A problem while dumping YAML data into the output stream (relevant
         * for `YAML_EMITTER_ERROR` and `YAML_SERIALIZER_ERROR`).
         */
        struct {
            /* The problem description. */
            const char *problem;
        } dumping;

        /*
         * A problem occured while resolving an implicit YAML node tag
         * (relevant for `YAML_RESOLVER_ERROR`).
         */
        struct {
            /* The problem description. */
            const char *problem;
        } resolving;

    } data;

} yaml_error_t;

/*
 * Generate an error message.
 *
 * Given an instance of the `yaml_error_t` structure and a buffer, the function
 * fills the buffer with a message describing the error.  The generated message
 * follows the pattern: `"Error type: error details"`.  If the buffer is not
 * large enough to hold the whole message, the function fails.
 *
 * Arguments:
 *
 * - `error`: an error object obtained using `yaml_parser_get_error()` or
 *   `yaml_emitter_get_error()`.
 *
 * - `buffer`: a pointer to a character buffer to be filled with a generated
 *   error message.
 *
 * - `capacity`: the size of the buffer.
 *
 * Returns: `1` if the function succeeded, `0` on error.  The function may fail
 * if the provided buffer is not large enough to hold the whole buffer, in
 * which case an application may increase the size of the buffer and call the
 * function again.  If the function fails, the content of the buffer is
 * undefined.
 */

YAML_DECLARE(int)
yaml_error_message(yaml_error_t *error, char *buffer, size_t capacity);

/******************************************************************************
 * Basic Types
 ******************************************************************************/

/*
 * The character type (UTF-8 octet).
 *
 * Usage of the string type `(yaml_char_t *)` in the LibYAML API indicates that
 * the string is encoded in UTF-8.
 */

typedef unsigned char yaml_char_t;

/*
 * The version directive information.
 *
 * Note that LibYAML only supports YAML 1.1.
 */

typedef struct yaml_version_directive_s {
    /* The major version number. */
    int major;
    /* The minor version number. */
    int minor;
} yaml_version_directive_t;

/*
 * The tag directive information.
 */

typedef struct yaml_tag_directive_s {
    /* The tag handle. */
    yaml_char_t *handle;
    /* The tag prefix. */
    yaml_char_t *prefix;
} yaml_tag_directive_t;

/*
 * The stream encoding.
 *
 * An application may explicitly specify the encoding in the input stream or
 * let the parser to detect the input stream encoding from a BOM mark.  If the
 * stream does not start with a BOM mark, UTF-8 is assumed.
 *
 * An application may specify the encoding of the output stream or let the
 * emitter to choose a suitable encoding, in which case, UTF-8 is used.
 */

typedef enum yaml_encoding_e {
    /* The default/autodetected encoding. */
    YAML_ANY_ENCODING,
    /* The UTF-8 encoding. */
    YAML_UTF8_ENCODING,
    /* The UTF-16-LE encoding. */
    YAML_UTF16LE_ENCODING,
    /* The UTF-16-BE encoding. */
    YAML_UTF16BE_ENCODING
} yaml_encoding_t;

/*
 * Line break types.
 *
 * An application may specify the line break type the emitter should use or
 * leave it to the emitter discretion.  In the latter case, LN (Unix style) is
 * used.
 */

typedef enum yaml_break_e {
    /* Let the parser choose the break type. */
    YAML_ANY_BREAK,
    /* Use CR for line breaks (Mac style). */
    YAML_CR_BREAK,
    /* Use LN for line breaks (Unix style). */
    YAML_LN_BREAK,
    /* Use CR LN for line breaks (DOS style). */
    YAML_CRLN_BREAK
} yaml_break_t;

/******************************************************************************
 * Node Styles
 ******************************************************************************/

/*
 * Scalar styles.
 *
 * There are two groups of scalar types in YAML: flow and block.  Flow scalars
 * are divided into three styles: plain, single-quoted, and double-quoted;
 * block scalars are divided into two styles: literal and folded.
 *
 * The parser reports the style in which a scalar node is represented, however
 * it is a purely presentation details that must not be used in interpreting
 * the node content.
 *
 * An application may suggest a preferred node style or leave it completely
 * to the emitter discretion.  Note that the emitter may ignore any stylistic
 * suggestions.
 */

typedef enum yaml_scalar_style_e {
    /* Let the emitter choose the style. */
    YAML_ANY_SCALAR_STYLE,

    /* The plain flow scalar style. */
    YAML_PLAIN_SCALAR_STYLE,

    /* The single-quoted flow scalar style. */
    YAML_SINGLE_QUOTED_SCALAR_STYLE,
    /* The double-quoted flow scalar style. */
    YAML_DOUBLE_QUOTED_SCALAR_STYLE,

    /* The literal block scalar style. */
    YAML_LITERAL_SCALAR_STYLE,
    /* The folded block scalar style. */
    YAML_FOLDED_SCALAR_STYLE
} yaml_scalar_style_t;

/*
 * Sequence styles.
 *
 * YAML supports two sequence styles: flow and block.
 *
 * The parser reports the style of a sequence node, but this information should
 * not be used in interpreting the sequence content.
 *
 * An application may suggest a preferred sequence style or leave it completely
 * to the emitter discretion.  Note that the emitter may ignore any stylistic
 * hints.
 */
typedef enum yaml_sequence_style_e {
    /* Let the emitter choose the style. */
    YAML_ANY_SEQUENCE_STYLE,

    /* The flow sequence style. */
    YAML_FLOW_SEQUENCE_STYLE
    /* The block sequence style. */
    YAML_BLOCK_SEQUENCE_STYLE,
} yaml_sequence_style_t;

/*
 * Mapping styles.
 *
 * YAML supports two mapping styles: flow and block.
 *
 * The parser reports the style of a mapping node, but this information should
 * not be used in interpreting the mapping content.
 *
 * An application may suggest a preferred mapping style or leave it completely
 * to the emitter discretion.  Note that the emitter may ignore any stylistic
 * hints.
 */

typedef enum yaml_mapping_style_e {
    /* Let the emitter choose the style. */
    YAML_ANY_MAPPING_STYLE,

    /* The block mapping style. */
    YAML_BLOCK_MAPPING_STYLE,
    /* The flow mapping style. */
    YAML_FLOW_MAPPING_STYLE
} yaml_mapping_style_t;

/******************************************************************************
 * Tokens
 ******************************************************************************/

/*
 * Token types.
 *
 * The LibYAML scanner generates the following types of tokens:
 *
 * - STREAM-START: indicates the beginning of the stream.
 *
 * - STREAM-END: indicates the end of the stream.
 *
 * - VERSION-DIRECTIVE: describes the `%YAML` directive.
 *
 * - TAG-DIRECTIVE: describes the `%TAG` directive.
 *
 * - DOCUMENT-START: the indicator `---`.
 *
 * - DOCUMENT-END: the indicator `...`.
 *
 * - BLOCK-SEQUENCE-START: indentation increase indicating the beginning of a
 *   block sequence node.
 *
 * - BLOCK-MAPPING-START: indentation increase indicating the beginning of a
 *   block mapping node.
 *
 * - BLOCK-END: indentation decrease indicating the end of a block collection
 *   node.
 *
 * - FLOW-SEQUENCE-START: the indicator `[`.
 *
 * - FLOW-SEQUENCE-END: the indicator `]`.
 *
 * - FLOW-MAPPING-START: the indicator `{`.
 *
 * - FLOW-MAPPING-END: the indicator `}`.
 *
 * - BLOCK-ENTRY: the beginning of an item of a block sequence.
 *
 * - FLOW-ENTRY: the beginning of an item of a flow sequence.
 *
 * - KEY: the beginning of a simple key, or the indicator `?`.
 *
 * - VALUE: the indicator `:`.
 *
 * - ALIAS: an alias of a node.
 *
 * - ANCHOR: a node anchor.
 *
 * - TAG: a node tag.
 *
 * - SCALAR: the content of a scalar node.
 */

typedef enum yaml_token_type_e {
    /* An empty unitialized token. */
    YAML_NO_TOKEN,

    /* A STREAM-START token. */
    YAML_STREAM_START_TOKEN,
    /* A STREAM-END token. */
    YAML_STREAM_END_TOKEN,

    /* A VERSION-DIRECTIVE token. */
    YAML_VERSION_DIRECTIVE_TOKEN,
    /* A TAG-DIRECTIVE token. */
    YAML_TAG_DIRECTIVE_TOKEN,
    /* A DOCUMENT-START token. */
    YAML_DOCUMENT_START_TOKEN,
    /* A DOCUMENT-END token. */
    YAML_DOCUMENT_END_TOKEN,

    /* A BLOCK-SEQUENCE-START token. */
    YAML_BLOCK_SEQUENCE_START_TOKEN,
    /* A BLOCK-SEQUENCE-END token. */
    YAML_BLOCK_MAPPING_START_TOKEN,
    /* A BLOCK-END token. */
    YAML_BLOCK_END_TOKEN,

    /* A FLOW-SEQUENCE-START token. */
    YAML_FLOW_SEQUENCE_START_TOKEN,
    /* A FLOW-SEQUENCE-END token. */
    YAML_FLOW_SEQUENCE_END_TOKEN,
    /* A FLOW-MAPPING-START token. */
    YAML_FLOW_MAPPING_START_TOKEN,
    /* A FLOW-MAPPING-END token. */
    YAML_FLOW_MAPPING_END_TOKEN,

    /* A BLOCK-ENTRY token. */
    YAML_BLOCK_ENTRY_TOKEN,
    /* A FLOW-ENTRY token. */
    YAML_FLOW_ENTRY_TOKEN,
    /* A KEY token. */
    YAML_KEY_TOKEN,
    /* A VALUE token. */
    YAML_VALUE_TOKEN,

    /* An ALIAS token. */
    YAML_ALIAS_TOKEN,
    /* An ANCHOR token. */
    YAML_ANCHOR_TOKEN,
    /* A TAG token. */
    YAML_TAG_TOKEN,
    /* A SCALAR token. */
    YAML_SCALAR_TOKEN
} yaml_token_type_t;

/*
 * The token object.
 *
 * Typically the token API is too low-level to be used directly by
 * applications.  A possible user of the token API is a syntax highlighting
 * application.
 */

typedef struct yaml_token_s {

    /* The token type. */
    yaml_token_type_t type;

    /* The token data. */
    union {

        /* Extra data associated with a STREAM-START token. */
        struct {
            /* The stream encoding. */
            yaml_encoding_t encoding;
        } stream_start;

        /* Extra data associated with a VERSION-DIRECTIVE token. */
        struct {
            /* The major version number. */
            int major;
            /* The minor version number. */
            int minor;
        } version_directive;

        /* Extra data associated with a TAG-DIRECTIVE token. */
        struct {
            /* The tag handle. */
            yaml_char_t *handle;
            /* The tag prefix. */
            yaml_char_t *prefix;
        } tag_directive;

        /* Extra data associated with an ALIAS token. */
        struct {
            /* The alias value. */
            yaml_char_t *value;
        } alias;

        /* Extra data associated with an ANCHOR token. */
        struct {
            /* The anchor value. */
            yaml_char_t *value;
        } anchor;

        /* Extra data associated with a TAG token. */
        struct {
            /* The tag handle. */
            yaml_char_t *handle;
            /* The tag suffix. */
            yaml_char_t *suffix;
        } tag;

        /* Extra data associated with a SCALAR token. */
        struct {
            /* The scalar value. */
            yaml_char_t *value;
            /* The length of the scalar value. */
            size_t length;
            /* The scalar style. */
            yaml_scalar_style_t style;
        } scalar;

    } data;

    /* The beginning of the token. */
    yaml_mark_t start_mark;
    /* The end of the token. */
    yaml_mark_t end_mark;

} yaml_token_t;

/*
 * Allocate a new empty token object.
 *
 * A token object allocated using this function must be deleted with
 * `yaml_token_delete()`.
 *
 * Returns: a new empty token object or `NULL` on error.  The function may fail
 * if it cannot allocate memory for a new token object.
 */

YAML_DECLARE(yaml_token_t *)
yaml_token_new(void);

/*
 * Deallocate a token object and free the associated data.
 *
 * A token object must be previously allocated with `yaml_token_new()`.
 *
 * Arguments:
 *
 * - `token`: a token object to be deallocated.
 */

YAML_DECLARE(void)
yaml_token_delete(yaml_token_t *token);

/*
 * Duplicate a token object.
 *
 * This function creates a deep copy of an existing token object.  It accepts
 * two token objects: an empty token and a model token.  The latter is supposed
 * to be initialized with `yaml_parser_parse_token()`.  The function assigns
 * the type of the model to the empty token as well as duplicates and copies
 * the internal state associated with the model token.
 *
 * Arguments:
 *
 * - `token`: an empty token object.
 *
 * - `model`: a token to be copied.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the state of the model token.  In that case,
 * the token remains empty.
 */

YAML_DECLARE(int)
yaml_token_duplicate(yaml_token_t *token, const yaml_token_t *model);

/*
 * Clear the token state.
 *
 * This function clears the type and the internal state of a token object
 * freeing any associated data.  After applying this function to a token, it
 * becomes empty.  It is supposed that the token was previously initialized
 * using `yaml_parser_parse_token()` or `yaml_token_duplicate()`.
 *
 * Arguments:
 *
 * - `token`: a token object.
 */

YAML_DECLARE(void)
yaml_token_clear(yaml_token_t *token);

/******************************************************************************
 * Events
 ******************************************************************************/

/*
 * Event types.
 *
 * The LibYAML parser generates, while the LibYAML emitter accepts, YAML events
 * of the following types:
 *
 * - STREAM-START: indicates the beginning of the stream.
 *
 * - STREAM-END: indicates the end of the stream.
 *
 * - DOCUMENT-START: indicates the beginning of the document.
 *
 * - DOCUMENT-END: indicates the end of the document.
 *
 * - ALIAS: an alias to an already produced node.
 *
 * - SCALAR: a scalar node.
 *
 * - SEQUENCE-START: indicates the beginning of a sequence node.
 *
 * - SEQUENCE-END: indicates the end of a sequence node.
 *
 * - MAPPING-START: indicates the beginning of a mapping node.
 *
 * - MAPPING-END: indicates the end of a mapping node.
 *
 * A valid sequence of events obeys the grammar:
 *
 *      stream ::= STREAM-START document* STREAM-END
 *      document ::= DOCUMENT-START node DOCUMENT-END
 *      node ::= ALIAS | SCALAR | sequence | mapping
 *      sequence ::= SEQUENCE-START node* SEQUENCE-END
 *      mapping ::= MAPPING-START (node node)* MAPPING-END
 */

typedef enum yaml_event_type_e {
    /* An empty unitialized event. */
    YAML_NO_EVENT,

    /* A STREAM-START event. */
    YAML_STREAM_START_EVENT,
    /* A STREAM-END event. */
    YAML_STREAM_END_EVENT,

    /* A DOCUMENT-START event. */
    YAML_DOCUMENT_START_EVENT,
    /* A DOCUMENT-END event. */
    YAML_DOCUMENT_END_EVENT,

    /* An ALIAS event. */
    YAML_ALIAS_EVENT,
    /* A SCALAR event. */
    YAML_SCALAR_EVENT,

    /* A SEQUENCE-START event. */
    YAML_SEQUENCE_START_EVENT,
    /* A SEQUENCE-END event. */
    YAML_SEQUENCE_END_EVENT,

    /* A MAPPING-START event. */
    YAML_MAPPING_START_EVENT,
    /* A MAPPING-END event. */
    YAML_MAPPING_END_EVENT
} yaml_event_type_t;

/*
 * The event object.
 *
 * The event-level API of LibYAML should be used for streaming applications.
 */

typedef struct yaml_event_s {

    /* The event type. */
    yaml_event_type_t type;

    /* The event data. */
    union {
        
        /* The stream parameters (for `YAML_STREAM_START_EVENT`). */
        struct {
            /* The document encoding. */
            yaml_encoding_t encoding;
        } stream_start;

        /* The document parameters (for `YAML_DOCUMENT_START_EVENT`). */
        struct {
            /* The version directive or `NULL` if not present. */
            yaml_version_directive_t *version_directive;

            /* The list of tag directives. */
            struct {
                /* The beginning of the list or `NULL`. */
                yaml_tag_directive_t *list;
                /* The length of the list. */
                size_t length;
                /* The capacity of the list (used internally). */
                size_t capacity;
            } tag_directives;

            /* Set if the document indicator is omitted. */
            int is_implicit;
        } document_start;

        /* The document end parameters (for `YAML_DOCUMENT_END_EVENT`). */
        struct {
            /* Set if the document end indicator is omitted. */
            int is_implicit;
        } document_end;

        /* The alias parameters (for `YAML_ALIAS_EVENT`). */
        struct {
            /* The anchor. */
            yaml_char_t *anchor;
        } alias;

        /* The scalar parameters (for `YAML_SCALAR_EVENT`). */
        struct {
            /* The node anchor or `NULL`. */
            yaml_char_t *anchor;
            /* The node tag or `NULL`. */
            yaml_char_t *tag;
            /* The scalar value. */
            yaml_char_t *value;
            /* The length of the scalar value (in bytes). */
            size_t length;
            /* Set if the tag is optional for the plain style. */
            int is_plain_nonspecific;
            /* Set if the tag is optional for any non-plain style. */
            int is_quoted_nonspecific;
            /* The scalar style. */
            yaml_scalar_style_t style;
        } scalar;

        /* The sequence parameters (for `YAML_SEQUENCE_START_EVENT`). */
        struct {
            /* The node anchor or `NULL`. */
            yaml_char_t *anchor;
            /* The node tag or `NULL`. */
            yaml_char_t *tag;
            /* Set if the tag is optional. */
            int is_nonspecific;
            /* The sequence style. */
            yaml_sequence_style_t style;
        } sequence_start;

        /* The mapping parameters (for `YAML_MAPPING_START_EVENT`). */
        struct {
            /* The node anchor or `NULL`. */
            yaml_char_t *anchor;
            /* The node tag or `NULL`. */
            yaml_char_t *tag;
            /* Set if the tag is optional. */
            int is_nonspecific;
            /* The mapping style. */
            yaml_mapping_style_t style;
        } mapping_start;

    } data;

    /* The beginning of the event. */
    yaml_mark_t start_mark;
    /* The end of the event. */
    yaml_mark_t end_mark;

} yaml_event_t;

/*
 * Allocate a new empty event object.
 *
 * An event object allocated using this function must be deleted with
 * `yaml_event_delete()`.
 *
 * Returns: a new empty event object or `NULL` on error.  The function may fail
 * if it cannot allocate memory for a new event object.
 */

YAML_DECLARE(yaml_event_t *)
yaml_event_new(void);

/*
 * Deallocate an event object and free the associated data.
 *
 * An event object must be previously allocated with `yaml_event_new()`.
 *
 * Arguments:
 *
 * - `event`: an event object to be deallocated.
 */

YAML_DECLARE(void)
yaml_event_delete(yaml_event_t *event);

/*
 * Duplicate an event object.
 *
 * This function creates a deep copy of an existing event object.  It accepts
 * two objects: an empty event and a model event.  The model event is supposed
 * to be initialized either with `yaml_parser_parse_event()` or using one of
 * the functions `yaml_event_create_*()`.  The function assigns the type of the
 * model to the empty event and copies the internal state associated with the
 * model event.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `model`: an event to be copied.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the state of the model event.  In that case,
 * the event remains empty.
 */

YAML_DECLARE(int)
yaml_event_duplicate(yaml_event_t *event, const yaml_event_t *model);

/*
 * Clear the event state.
 *
 * This function clears the type and the internal state of an event object
 * freeing any associated data.  After applying this function to an event, it
 * becomes empty.  It is supposed that the event was previously initialized
 * using `yaml_parser_parse_event()` or `yaml_event_duplicate()`.  Note that
 * the function `yaml_emitter_emit_event()` also clears the given event.
 *
 * Arguments:
 *
 * - `event`: an event object.
 */

YAML_DECLARE(void)
yaml_event_clear(yaml_event_t *event);

/*
 * Create a STREAM-START event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `encoding`: the stream encoding.
 *
 * Returns: `1`.  The function never fails.
 */

YAML_DECLARE(int)
yaml_event_create_stream_start(yaml_event_t *event,
        yaml_encoding_t encoding);

/*
 * Create a STREAM-END event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * Returns: `1`.  The function never fails.
 */

YAML_DECLARE(int)
yaml_event_create_stream_end(yaml_event_t *event);

/*
 * Create a DOCUMENT-START event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `version_directive`: a structure specifying the content of the `%YAML`
 *   directive or `NULL` if the directive could be omitted.  Note that LibYAML
 *   supports YAML 1.1 only.  The function does not check if the supplied
 *   version equals to 1.1, but the emitter will fail to emit the event if it
 *   is not so.
 *
 * - `tag_directives_list`: a pointer to a list specifying the content of the
 *   `%TAG` directives associated with the document or `NULL` if the document
 *   does not contain `%TAG` directives.  The content of a tag directive is a
 *   pair (handle, prefix) of non-empty NUL-terminated UTF-8 strings.  The tag
 *   handle is one of `!`, `!!` or a sequence of alphanumeric characters, `_`
 *   and `-` surrounded by `!`.  The tag prefix is a prefix of any valid tag,
 *   that is, it is a non-empty prefix of either a global tag (a valid URI) or
 *   a local tag (an arbitrary string starting with `!`).  The function does
 *   not check if the given directive values satisfy these requirements, but
 *   the emitter will fail to emit the event if they are not met.
 *
 * - `tag_directives_length`: the length of `tag_directives_list`; `0` if
 *   `tag_directives_list` is `NULL`.
 *
 * - `is_implicit`: `1` if the document indicator `---` is omitted, `0`
 *   otherwise.  Note that this attribute is only a stylistic hint and could be
 *   ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the event parameters.  In this case, the
 * event remains empty.
 */

YAML_DECLARE(int)
yaml_event_create_document_start(yaml_event_t *event,
        const yaml_version_directive_t *version_directive,
        const yaml_tag_directive_t *tag_directives_list,
        size_t tag_directives_length,
        int is_implicit);

/*
 * Create a DOCUMENT-END event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `is_implicit`: `1` if the document end indicator `...` is omitted, `0`
 *   otherwise.  Note that this attribute is only a stylistic hint and could be
 *   ignored by the emitter.
 *
 * Returns: `1`.  The function never fails.
 */

YAML_DECLARE(int)
yaml_event_create_document_end(yaml_event_t *event, int is_implicit);

/*
 * Create an ANCHOR event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `anchor`: the anchor value.  The anchor should be a non-empty
 *   NUL-terminated string containing only alphanumerical characters, `_`, and
 *   `-`.  The function does not check if this requirement is satisfied, but if
 *   it is not so, the emitter will fail to emit the generated event.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating `anchor`.  In this case, the event remains
 * empty.
 */

YAML_DECLARE(int)
yaml_event_create_alias(yaml_event_t *event, const yaml_char_t *anchor);

/*
 * Create a SCALAR event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `anchor`: the anchor value or `NULL`.  The anchor should be a non-empty
 *   NUL-terminated string containing only alphanumerical characters, `_`, and
 *   `-`.
 *
 * - `tag`: the tag value or `NULL`.  The tag should be a non-empty UTF-8
 *   NUL-terminated string.  The tag could be global (a valid URI) or local (an
 *   arbitrary string starting with `!`).  If `NULL` is provided, at least one
 *   of the flags `is_plain_nonspecific` and `is_quoted_nonspecific` must be
 *   set.  The function does not check whether these requirements are
 *   satisfied, but the emitter will fail to emit the event if it is not so.
 *
 * - `value`: the value of the scalar node.  The value should be a UTF-8
 *   string.  It could contain any valid UTF-8 character including NUL.  The
 *   function does not check if `value` is a valid UTF-8 string, but the
 *   emitter will fail to emit the event if it is not so.
 *
 * - `length`: the length of the scalar value (in bytes) or `-1`.  If `length`
 *   is set to `-1`, the `value` is assumed to be NUL-terminated.
 *
 * - `is_plain_nonspecific`: `1` if the node tag could be omitted while
 *   emitting the node provided that the the plain style is used for
 *   representing the node value; `0` otherwise.  That this flag is set assumes
 *   that the tag could be correctly determined by the parser using the node
 *   position and content.
 *
 * - `is_quoted_nonspecific`: `1` if the node tag could be omitted while
 *   emitting the node provided that the node value is represented using any
 *   non-plain style; `0` otherwise.  That this flag is set assumes that the
 *   tag could be correctly determined by the parser using the node position
 *   and content.
 *
 * - `style`: the node style.  Note that this attribute only serves as a hint
 *   and may be ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the given string buffers.  In this case, the
 * event remains empty.
 */

YAML_DECLARE(int)
yaml_event_create_scalar(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        const yaml_char_t *value, int length,
        int is_plain_nonspecific, int is_quoted_nonspecific,
        yaml_scalar_style_t style);

/*
 * Create a SEQUENCE-START event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `anchor`: the anchor value or `NULL`.  The anchor should be a non-empty
 *   NUL-terminated string containing only alphanumerical characters, `_`, and
 *   `-`.  The function does not check if this requirement is satisfied, but if
 *   it is not so, the emitter will fail to emit the generated event.
 *
 * - `tag`: the tag value or `NULL`.  The tag should be a non-empty UTF-8
 *   NUL-terminated string.  The tag could be global (a valid URI) or local (an
 *   arbitrary string starting with `!`).  If `NULL` is provided, the flag
 *   `is_nonspecific` must be set.  The function does not check whether these
 *   requirements are satisfied, but if it is not so, the emitter will fail to
 *   emit the generated event.
 *
 * - `is_nonspecific`: `1` if the node tag could be omitted while
 *   emitting the node; `0` otherwise.  This flag should only be set if the
 *   node tag could be correctly determined by the parser using the node
 *   position in the document graph.
 *
 * - `style`: the node style.  Note that this attribute only serves as a hint
 *   and may be ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the given string buffers.  In this case, the
 * event remains empty.
 */

YAML_DECLARE(int)
yaml_event_create_sequence_start(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        int is_nonspecific, yaml_sequence_style_t style);

/*
 * Create a SEQUENCE-END event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * Returns: `1`.  This function never fails.
 */

YAML_DECLARE(int)
yaml_event_create_sequence_end(yaml_event_t *event);

/*
 * Create a MAPPING-START event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * - `anchor`: the anchor value or `NULL`.  The anchor should be a non-empty
 *   NUL-terminated string containing only alphanumerical characters, `_`, and
 *   `-`.  The function does not check if this requirement is satisfied, but if
 *   it is not so, the emitter will fail to emit the generated event.
 *
 * - `tag`: the tag value or `NULL`.  The tag should be a non-empty UTF-8
 *   NUL-terminated string.  The tag could be global (a valid URI) or local (an
 *   arbitrary string starting with `!`).  If `NULL` is provided, the flag
 *   `is_nonspecific` must be set.  The function does not check whether these
 *   requirements are satisfied, but if it is not so, the emitter will fail to
 *   emit the generated event.
 *
 * - `is_nonspecific`: `1` if the node tag could be omitted while
 *   emitting the node; `0` otherwise.  This flag should only be set if the
 *   node tag could be correctly determined by the parser using the node
 *   position in the document graph.
 *
 * - `style`: the node style.  Note that this attribute only serves as a hint
 *   and may be ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the given string buffers.  In this case, the
 * event remains empty.
 */

YAML_DECLARE(int)
yaml_event_create_mapping_start(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        int is_nonspecific, yaml_mapping_style_t style);

/*
 * Create a MAPPING-END event.
 *
 * This function initializes an empty event object allocated with
 * `yaml_event_new()`.  The initialized event could be fed to
 * `yaml_emitter_emit_event()`.
 *
 * Arguments:
 *
 * - `event`: an empty event object.
 *
 * Returns: `1`.  This function never fails.
 */

YAML_DECLARE(int)
yaml_event_create_mapping_end(yaml_event_t *event);

/******************************************************************************
 * Documents and Nodes
 ******************************************************************************/

/*
 * Well-known scalar tags.
 */

#define YAML_NULL_TAG       ((const yaml_char_t *) "tag:yaml.org,2002:null")
#define YAML_BOOL_TAG       ((const yaml_char_t *) "tag:yaml.org,2002:bool")
#define YAML_STR_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:str")
#define YAML_INT_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:int")
#define YAML_FLOAT_TAG      ((const yaml_char_t *) "tag:yaml.org,2002:float")

/*
 * Basic collection tags.
 */

#define YAML_SEQ_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:seq")
#define YAML_MAP_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:map")

/*
 * The default tags for nodes lacking an explicit tag.
 */

#define YAML_DEFAULT_SCALAR_TAG     YAML_STR_TAG
#define YAML_DEFAULT_SEQUENCE_TAG   YAML_SEQ_TAG
#define YAML_DEFAULT_MAPPING_TAG    YAML_MAP_TAG

/*
 * Document types.
 *
 * There are no different document types in LibYAML: the document type field is
 * only used to distinguish between newly allocated documents and those that
 * are initialized with `yaml_parser_parse_document()` or
 * `yaml_document_create()`.
 */

typedef enum yaml_document_type_e {
    /* An empty uninitialized document. */
    YAML_NO_DOCUMENT.

    /* A YAML document. */
    YAML_DOCUMENT
} yaml_document_type_t;

/*
 * Node types.
 *
 * YAML recognizes three kinds of nodes: scalar, sequence and mapping.
 */

typedef enum yaml_node_type_e {
    /* An empty node. */
    YAML_NO_NODE,

    /* A scalar node. */
    YAML_SCALAR_NODE,
    /* A sequence node. */
    YAML_SEQUENCE_NODE,
    /* A mapping node. */
    YAML_MAPPING_NODE
} yaml_node_type_t;

/*
 * Arc types.
 *
 * Arcs are used to specify the path from the root node to a given node in the
 * document graph.  There are three kinds of arcs: an item in a sequence node,
 * a key in a mapping node, and a value in a mapping node.
 */

typedef enum yaml_arc_type_e {
    /* An empty arc. */
    YAML_NO_ARC,

    /* An item of a sequence. */
    YAML_SEQUENCE_ITEM_ARC,
    /* A key of a mapping. */
    YAML_MAPPING_KEY_ARC,
    /* A value of a mapping. */
    YAML_MAPPING_VALUE_ARC
} yaml_arc_type_t;

/*
 * An element of a sequence node.
 */

typedef int yaml_node_item_t;

/*
 * An element of a mapping node.
 */

typedef struct yaml_node_pair_s {
    /* A key in a mapping. */
    int key;
    /* A value in a mapping. */
    int value;
} yaml_node_pair_t;

/*
 * A path element.
 *
 * An arc is an element of a path from the root node to some other node in a
 * YAML document.  An arc consists of a collection node and information on how
 * it is connected to the next node in the path.  If the arc type is a sequence
 * item, then the collection node is a sequence and the arc refers to the index
 * in this sequence.  If the arc type is a mapping key, then the collection
 * node is a mapping.  If the arc type is a mapping value, then the collection
 * node is a mapping and the arc refers to the key associated to the value.
 */

typedef struct yaml_arc_s {

    /* The arc type. */
    yaml_arc_type_t type;

    /* The tag of the collection node. */
    yaml_char_t *tag;

    /* The connection information. */
    union {

        /* The sequence item data (for `YAML_SEQUENCE_ITEM_ARC`). */
        struct {
            /* The index of the item in the sequence (starting from `0`). */
            int index;
        } item;

        /* The mapping value data (for `YAML_MAPPING_VALUE_ARC`). */
        struct {
            /* The key associated with the value. */
            struct {
                /* The key node type. */
                yaml_node_type_t type;
                /* The key node tag. */
                yaml_char_t *tag;
                /* The key node details. */
                union {
                    /* The scalar data (for a scalar key). */
                    struct {
                        /* The scalar value. */
                        yaml_char_t *value;
                        /* The scalar length. */
                        size_t length;
                    } scalar;
                } data;
            } key;
        } value;
    } data;
} yaml_arc_t;

/*
 * The node object.
 *
 * A node object represents a node in a YAML document graph.  Node objects are
 * created using the family of functions `yaml_document_add_*()`.  Links
 * between nodes are created using the functions `yaml_document_append_*()`. A
 * node object is destroyed when the document containing it is destroyed.
 */

struct yaml_node_s {

    /* The node type. */
    yaml_node_type_t type;

    /* The node anchor or `NULL`. */
    yaml_char_t *anchor;
    /* The node tag. */
    yaml_char_t *tag;

    /* The node data. */
    union {
        
        /* The scalar parameters (for `YAML_SCALAR_NODE`). */
        struct {
            /* The scalar value. */
            yaml_char_t *value;
            /* The length of the scalar value. */
            size_t length;
            /* The scalar style. */
            yaml_scalar_style_t style;
        } scalar;

        /* The sequence parameters (for `YAML_SEQUENCE_NODE`). */
        struct {
            /* The list of sequence items. */
            struct {
                /* The pointer to the beginning of the list. */
                yaml_node_item_t *list;
                /* The length of the list. */
                size_t length;
                /* The capacity of the list (used internally). */
                size_t capacity;
            } items;
            /* The sequence style. */
            yaml_sequence_style_t style;
        } sequence;

        /* The mapping parameters (for `YAML_MAPPING_NODE`). */
        struct {
            /* The list of mapping pairs (key, value). */
            struct {
                /** The pointer to the beginning of the list. */
                yaml_node_pair_t *list;
                /* The length of the list. */
                size_t length;
                /* The capacity of the list (used internally). */
                size_t capacity;
            } pairs;
            /* The mapping style. */
            yaml_mapping_style_t style;
        } mapping;

    } data;

    /* The beginning of the node. */
    yaml_mark_t start_mark;
    /* The end of the node. */
    yaml_mark_t end_mark;

};

/*
 * The incomplete node object.
 *
 * This structure provides the information about a node that a tag resolver
 * could use to determine the specific node tag.  This information includes
 * the path from the root node and the node content for scalar nodes.
 */

typedef struct yaml_incomplete_node_s {

    /* The node type. */
    yaml_node_type_t type;

    /* The path to the new node. */
    struct {
        /* The pointer to the beginning of the list. */
        yaml_arc_t *list;
        /* The length of the list. */
        size_t length;
        /* The capacity of the list (used internally). */
        size_t capacity;
    } path;

    /* The node data. */
    union {

        /* The scalar parameters (for `YAML_SCALAR_NODE`). */
        struct {
            /* The scalar value. */
            yaml_char_t *value;
            /* The length of the scalar value. */
            size_t length;
            /* Set if the scalar is plain. */
            int is_plain;
        } scalar;

    } data;

    /* The position of the node. */
    yaml_mark_t mark;

} yaml_incomplete_node_t;

/*
 * The document object.
 *
 * A document object represents the main structure of the YAML object model:
 * the document graph consisting of nodes of three kinds: scalars, sequences,
 * and mappings with the selected root node.
 *
 * An empty document object is allocated using the function
 * `yaml_document_new()`.  Then the function `yaml_parser_parse_document()`
 * could be used to load a YAML document from the input stream.  Alternatively,
 * a document could be created with `yaml_document_create()` and its content
 * could be specified using the families of functions `yaml_document_add_*()`
 * and `yaml_document_append_*()`.  A document with all associated nodes could
 * be destroyed using the function `yaml_document_delete()`.
 */

typedef struct yaml_document_s {

    /* The document type. */
    yaml_document_type_t type;

    /* The document nodes (for internal use only). */
    struct {
        /* The pointer to the beginning of the list. */
        yaml_node_t *list;
        /* The length of the list. */
        size_t length;
        /* The capacity of the list. */
        size_t capacity;
    } nodes;

    /* The version directive or `NULL`. */
    yaml_version_directive_t *version_directive;

    /* The list of tag directives. */
    struct {
        /* The pointer to the beginning of the list or `NULL`. */
        yaml_tag_directive_t *list;
        /** The length of the list. */
        size_t length;
        /** The capacity of the list (used internally). */
        size_t capacity;
    } tag_directives;

    /** Set if the document start indicator is implicit. */
    int is_start_implicit;
    /** Set if the document end indicator is implicit. */
    int is_end_implicit;

    /** The beginning of the document. */
    yaml_mark_t start_mark;
    /** The end of the document. */
    yaml_mark_t end_mark;

} yaml_document_t;

/*
 * Allocate a new empty document object.
 *
 * A document object allocated using this function must be deleted with
 * `yaml_document_delete()`.
 *
 * Returns: a new empty document object or `NULL` on error.  The function may
 * fail if it cannot allocate memory for a new object.
 */

YAML_DECLARE(yaml_document_t *)
yaml_document_new(void);

/*
 * Deallocate a document object and free the associated data.
 *
 * A document object must be previously allocated with `yaml_document_new()`.
 *
 * Arguments:
 *
 * - `document`: a document object to be deallocated.
 */

YAML_DECLARE(void)
yaml_document_delete(yaml_document_t *document);

/*
 * Duplicate a document object.
 *
 * This function creates a complete copy of an existing document and all the
 * nodes it contains.  The function accepts two objects: an empty document and
 * a model document.  The model is supposed to be initialized either with
 * `yaml_parser_parse_document()` or constructed manually.  The functions
 * duplicate the content of the document and its nodes and assigns it to the
 * empty document.
 *
 * Arguments:
 *
 * - `document`: an empty document object.
 *
 * - `model`: a document to be copied.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the state of the model.  In that case, the
 * document remains empty.
 */

YAML_DECLARE(int)
yaml_document_duplicate(yaml_document_t *document, yaml_document_t *model);

/*
 * Clear the document.
 *
 * This function clears the type of the document and destroys all the document
 * nodes.  After applying this function to a document, it becomes empty.  It is
 * supposed that the document was previously initialized using
 * `yaml_parser_parse_document()` or created manually.  Note that the function
 * `yaml_emitter_emit_document()` also clears the given document.
 *
 * Arguments:
 *
 * - `document`: a document object.
 */

YAML_DECLARE(void)
yaml_document_clear(yaml_document_t *document);

/*
 * Create a YAML document.
 *
 * This function initializes an empty document object allocated with
 * `yaml_document_new()`.  Further, nodes could be added to the document using
 * the functions `yaml_document_add_*()` and `yaml_document_append_*()`.  The
 * initialized document could be fed to `yaml_emitter_emit_document()`.
 *
 * Arguments:
 *
 * - `document`: an empty document object.
 *
 * - `version_directive`: a structure specifying the content of the `%YAML`
 *   directive or `NULL` if the directive could be omitted.  Note that LibYAML
 *   supports YAML 1.1 only.  The constructor does not check if the supplied
 *   version equals to 1.1, but the emitter will fail to emit the document if
 *   it is not so.
 *
 * - `tag_directives_list`: a pointer to a list specifying the content of the
 *   `%TAG` directives associated with the document or `NULL` if the document
 *   does not contain `%TAG` directives.  The content of a tag directive is a
 *   pair (handle, prefix) of non-empty NUL-terminated UTF-8 strings.  The tag
 *   handle is one of `!`, `!!` or a sequence of alphanumeric characters, `_`
 *   and `-` surrounded by `!`.  The tag prefix is a prefix of any valid tag,
 *   that is, it is a non-empty prefix of either a global tag (a valid URI) or
 *   a local tag (an arbitrary string starting with `!`).  The constructor does
 *   not check if the given directive values satisfy these requirements, but
 *   the emitter will fail to emit the document if they are not met.
 *
 * - `tag_directives_length`: the length of `tag_directives_list`; `0` if
 *   `tag_directives_list` is `NULL`.
 *
 * - `is_start_implicit`: `1` if the document start indicator `---` is omitted;
 *   `0` otherwise.  Note that this attribute is only a stylistic hint and
 *   could be ignored by the emitter.
 *
 * - `is_end_implicit`: `1` if the document end indicator `...` is omitted; `0`
 *   otherwise.  Note that this attribute is only a stylistic hint and could be
 *   ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the document parameters.  In this case, the
 * document remains empty.
 */

YAML_DECLARE(int)
yaml_document_create(yaml_document_t *document,
        const yaml_version_directive_t *version_directive,
        const yaml_tag_directive_t *tag_directives_list,
        size_t tag_directives_length,
        int is_start_implicit, int is_end_implicit);

/**
 * Get a node of a YAML document.
 *
 * The root node of the document has the id `0`.
 *
 * The pointer returned by this function is valid until any of the functions
 * modifying the document is called.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id.  The node id starts from `0` and increases by `1`
 *   for each newly added node.  Specifying a negative `node_id` is possible,
 *   which is interpeted as the number (<total number of nodes> - `node_id`).
 *
 * Returns: a pointer to the node object or `NULL` if `node_id` is out of
 * range.
 */

YAML_DECLARE(yaml_node_t *)
yaml_document_get_node(yaml_document_t *document, int node_id);

/*
 * Create a SCALAR node and attach it to the document.
 *
 * Note that the first node attached to a document becomes the root node.
 * There must exist a path from the root node to any node added to the
 * document, otherwise the function `yaml_emitter_emit_document()` will fail.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * - `anchor`: the node anchor or `NULL`.  The anchor should be a non-empty
 *   NUL-terminated string containing only alphanumerical characters, `_`, and
 *   `-`.  The function does not check whether this requirement is satisfied,
 *   but the emitter may fail to emit the node if it is not so.  This parameter
 *   is considered as a stylistic hint and could be ignored by the emitter.
 *   The emitter may automatically generate an anchor for a node that does not
 *   specify it or specifies a duplicate anchor.
 *
 * - `tag`: the node tag.  The tag should be a non-empty UTF-8 NUL-terminated
 *   string.  The tag could be global (a valid URI) or local (an arbitrary
 *   string starting with `!`).  The function does not check whether these
 *   requirements are satisfied, but the emitter will fail to emit the node if
 *   it is not so.
 *
 * - `value`: the scalar value.  The value should be a string containing any
 *   valid UTF-8 character including NUL.  The function does not check if
 *   `value` is a valid UTF-8 string, but the emitter will fail to emit the
 *   node if it is not so.
 *
 * - `length`: the length of the scalar value (in bytes) or `-1`.  If `length`
 *   is set to `-1`, the `value` is assumed to be NUL-terminated.
 *
 * - `style`: the node style.  Note that this attribute only serves as a hint
 *   and may be ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the given string buffers.  If the function
 * succeeds, the id of the added node is returned via the pointer `node_id`
 * if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_scalar(yaml_document_t *document, int *node_id,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        const yaml_char_t *value, int length,
        yaml_scalar_style_t style);

/*
 * Create a SEQUENCE node and attach it to the document.
 *
 * Note that the first node attached to a document becomes the root node.
 * There must exist a path from the root node to any node added to the
 * document, otherwise the function `yaml_emitter_emit_document()` will fail.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * - `anchor`: the node anchor or `NULL`.  The anchor should be a non-empty
 *   NUL-terminated string containing only alphanumerical characters, `_`, and
 *   `-`.  The function does not check whether this requirement is satisfied,
 *   but the emitter may fail to emit the node if it is not so.  This parameter
 *   is considered as a stylistic hint and could be ignored by the emitter.
 *   The emitter may automatically generate an anchor for a node that does not
 *   specify it or specifies a duplicate anchor.
 *
 * - `tag`: the node tag.  The tag should be a non-empty UTF-8 NUL-terminated
 *   string.  The tag could be global (a valid URI) or local (an arbitrary
 *   string starting with `!`).  The function does not check whether these
 *   requirements are satisfied, but the emitter will fail to emit the node if
 *   it is not so.
 *
 * - `style`: the node style.  Note that this attribute only serves as a hint
 *   and may be ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the given string buffers.  If the function
 * succeeds, the id of the added node is returned via the pointer `node_id`
 * if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_sequence(yaml_document_t *document, int *node_id,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        yaml_sequence_style_t style);

/*
 * Create a MAPPING node and attach it to the document.
 *
 * Note that the first node attached to a document becomes the root node.
 * There must exist a path from the root node to any node added to the
 * document, otherwise the function `yaml_emitter_emit_document()` will fail.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * - `anchor`: the node anchor or `NULL`.  The anchor should be a non-empty
 *   NUL-terminated string containing only alphanumerical characters, `_`, and
 *   `-`.  The function does not check whether this requirement is satisfied,
 *   but the emitter may fail to emit the node if it is not so.  This parameter
 *   is considered as a stylistic hint and could be ignored by the emitter.
 *   The emitter may automatically generate an anchor for a node that does not
 *   specify it or specifies a duplicate anchor.
 *
 * - `tag`: the node tag.  The tag should be a non-empty UTF-8 NUL-terminated
 *   string.  The tag could be global (a valid URI) or local (an arbitrary
 *   string starting with `!`).  The function does not check whether these
 *   requirements are satisfied, but the emitter will fail to emit the node if
 *   it is not so.
 *
 * - `style`: the node style.  Note that this attribute only serves as a hint
 *   and may be ignored by the emitter.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for duplicating the given string buffers.  If the function
 * succeeds, the id of the added node is returned via the pointer `node_id`
 * if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_mapping(yaml_document_t *document, int *node_id,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        yaml_mapping_style_t style);

/*
 * Get the value of a `!!null` SCALAR node.
 *
 * Use this function to ensure that the given node is a scalar, the node tag is
 * equal to `tag:yaml.org,2002:null` and the node value is a valid null value.
 * Given that the `!!null` tag admits only one valid value, the value is not
 * returned.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id; could be negative.
 *
 * Returns: `1` if the node is a valid `!!null` scalar, `0` otherwise.
 */

YAML_DECLARE(int)
yaml_document_get_null_node(yaml_document_t *document, int node_id);

/*
 * Get the value of a `!!bool` SCALAR node.
 *
 * Use this function to ensure that the given node is a scalar, the node tag is
 * `tag:yaml.org,2002:bool` and the node value is a valid boolean value.  The
 * function returns the true value as `1` and the false value as `0`.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id; could be negative.
 *
 * - `value`: a pointer to save the node value or `NULL`.
 *
 * Returns: `1` if the node is a valid `!!bool` scalar, `0` otherwise.  If the
 * function succeeds and `value` is not `NULL`, the node value is saved to
 * `value`.
 */

YAML_DECLARE(int)
yaml_document_get_bool_node(yaml_document_t *document, int node_id,
        int *value);

/*
 * Get the value of a `!!str` SCALAR node.
 *
 * Use this function to ensure that the given node is a scalar, the node tag is
 * `tag:yaml.org,2002:str` and the node value is a string that does not contain
 * the NUL character.  In this case, the function returns the node value.  The
 * produced value is valid until the document object is cleared or deleted.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id; could be negative.
 *
 * - `value`: a pointer to save the node value or `NULL`.
 *
 * Returns: `1` if the node is a valid `!!str` scalar, `0` otherwise.  If the
 * function succeeds and `value` is not `NULL`, the node value is saved to
 * `value`.
 */

YAML_DECLARE(int)
yaml_document_get_str_node(yaml_document_t *document, int node_id,
        char **value);

/*
 * Get the value of an `!!int` SCALAR node.
 *
 * Use this function to ensure that the given node is a scalar, the node tag is
 * `tag:yaml.org,2002:int` and the node value is a valid integer.  In this
 * case, the function parses the node value and returns an integer number.  The
 * function recognizes decimal, hexdecimal and octal numbers including negative
 * numbers.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id; could be negative.
 *
 * - `value`: a pointer to save the node value or `NULL`.
 *
 * Returns: `1` if the node is a valid `!!int` scalar, `0` otherwise.  If the
 * function succeeds and `value` is not `NULL`, the node value is saved to
 * `value`.
 */

YAML_DECLARE(int)
yaml_document_get_int_node(yaml_document_t *document, int node_id,
        int *value);

/*
 * Get the value of a `!!float` SCALAR node.
 *
 * Use this function to ensure that the given node is a scalar, the node tag is
 * `tag:yaml.org,2002:float` and the node value is a valid float value.  In
 * this case, the function parses the node value and returns a float number.
 * The function recognizes float values in exponential and fixed notation as
 * well as special values `.nan`, `.inf` and `-.inf`.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id; could be negative.
 *
 * - `value`: a pointer to save the node value or `NULL`.
 *
 * Returns: `1` if the node is a valid `!!float` scalar, `0` otherwise.  If the
 * function succeeds and `value` is not `NULL`, the node value is saved to
 * `value`.
 */

YAML_DECLARE(int)
yaml_document_get_float_node(yaml_document_t *document, int node_id,
        double *value);

/*
 * Get the value of a `!!seq` SEQUENCE node.
 *
 * Use this function to ensure that the given node is a sequence and the node
 * tag is `tag:yaml.org,2002:seq`.  In this case, the function returns the list
 * of nodes that belong to the sequence.  The produced list is valid until the
 * document object is modified.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id; could be negative.
 *
 * - `items`: a pointer to save the list of sequence items or `NULL`.
 *
 * - `length`: a pointer to save the length of the sequence or `NULL`.
 *   `length` must be equal to `NULL` if and only if `items` is also `NULL`.
 *
 * Returns: `1` if the node is a valid `!!seq` sequence, `0` otherwise.  If the
 * function succeeds and `items` is not `NULL`, the list of sequence items is
 * saved to `items` and the sequence length is saved to `length`.
 */

YAML_DECLARE(int)
yaml_document_get_seq_node(yaml_document_t *document, int node_id,
        yaml_node_item_t **items, size_t *length);

/*
 * Get the value of a `!!map` MAPPING node.
 *
 * Use this function to ensure that the given node is a mapping and the node
 * tag is `tag:yaml.org,2002:map`.  In this case, the function returns the list
 * of node pairs (key, value) that belong to the sequence.  The produced list
 * is valid until the document is modified.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: the node id; could be negative.
 *
 * - `pairs`: a pointer to save the list of mapping pairs or `NULL`.
 *
 * - `length`: a pointer to save the length of the mapping or `NULL`.
 *   `length` must be equal to `NULL` if and only if `pairs` is also `NULL`.
 *
 * Returns: `1` if the node is a valid `!!map` mapping, `0` otherwise.  If the
 * function succeeds and `pairs` is not `NULL`, the list of mapping pairs is
 * saved to `pairs` and the mapping length is saved to `length`.
 */

YAML_DECLARE(int)
yaml_document_get_map_node(yaml_document_t *document, int node_id,
        yaml_node_pair_t **pairs, size_t *length);

/*
 * Add a `!!null` SCALAR node to the document.
 *
 * This function is a shorthand for the call:
 *
 *      yaml_document_add_scalar(document, node_id, NULL,
 *              YAML_NULL_TAG, "null", -1, YAML_ANY_SCALAR_STYLE)
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for new buffers.  If the function succeeds, the id of the
 * added node is returned via the pointer `node_id` if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_null_node(yaml_document_t *document, int *node_id);

/*
 * Add a `!!bool` SCALAR node to the document.
 *
 * This function is a shorthand for the call:
 *
 *      yaml_document_add_scalar(document, node_id, NULL,
 *              YAML_BOOL_TAG, (value ? "true" : "false"), -1,
 *              YAML_ANY_SCALAR_STYLE)
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * - `value`: a boolean value; any non-zero value is true, `0` is false.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for new buffers.  If the function succeeds, the id of the
 * added node is returned via the pointer `node_id` if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_bool_node(yaml_document_t *document, int *node_id,
        int value);

/*
 * Add a `!!str` SCALAR node to the document.
 *
 * This function is a shorthand for the call:
 *
 *      yaml_document_add_scalar(document, node_id, NULL,
 *              YAML_STR_TAG, (const yaml_char_t *) value, -1,
 *              YAML_ANY_SCALAR_STYLE)
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * - `value`: a NUL-terminated UTF-8 string.  The function does not check if
 *   `value` is a valid UTF-8 string, but if it is not so, the emitter will
 *   fail to emit the node.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for new buffers.  If the function succeeds, the id of the
 * added node is returned via the pointer `node_id` if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_str_node(yaml_document_t *document, int *node_id,
        const char *value);

/*
 * Add an `!!int` SCALAR node to the document.
 *
 * This function is a shorthand for the call:
 *
 *      yaml_document_add_scalar(document, node_id, NULL,
 *              YAML_INT_TAG, <string representation of the value>, -1,
 *              YAML_ANY_SCALAR_STYLE)
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * - `value`: an integer value.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for new buffers.  If the function succeeds, the id of the
 * added node is returned via the pointer `node_id` if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_int_node(yaml_document_t *document, int *node_id,
        int value);

/*
 * Add a `!!float` SCALAR node to the document.
 *
 * This function is a shorthand for the call:
 *
 *      yaml_document_add_scalar(document, node_id, NULL,
 *              YAML_FLOAT_TAG, <string representation of the value>, -1,
 *              YAML_ANY_SCALAR_STYLE)
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * - `value`: a float value.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for new buffers.  If the function succeeds, the id of the
 * added node is returned via the pointer `node_id` if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_float_node(yaml_document_t *document, int *node_id,
        double value);

/*
 * Add a `!!seq` SEQUENCE node to the document.
 *
 * This function is a shorthand for the call:
 *
 *      yaml_document_add_sequence(document, node_id, NULL,
 *              YAML_SEQ_TAG, YAML_ANY_SEQUENCE_STYLE)
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for new buffers.  If the function succeeds, the id of the
 * added node is returned via the pointer `node_id` if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_seq_node(yaml_document_t *document, int *node_id);

/*
 * Add a `!!map` MAPPING node to the document.
 *
 * This function is a shorthand for the call:
 *
 *      yaml_document_add_mapping(document, node_id, NULL,
 *              YAML_MAP_TAG, YAML_ANY_MAPPING_STYLE)
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `node_id`: a pointer to save the id of the generated node or `NULL`.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for new buffers.  If the function succeeds, the id of the
 * added node is returned via the pointer `node_id` if it is not set to `NULL`.
 */

YAML_DECLARE(int)
yaml_document_add_map_node(yaml_document_t *document, int *node_id);

/*
 * Add an item to a SEQUENCE node.
 *
 * The order in which items are added to a sequence coincides with the order
 * they are emitted into the output stream.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `sequence_id`: the id of a sequence node; could be negative.  It is a
 *   fatal error if `sequence_id` does not refer to an existing sequence node.
 *
 * - `item_id`: the id of an item node; could be negative.  It is a fatal error
 *   if `item_id` does not refer to an existing node.  Note that it is possible
 *   for `item_id` to coincide with `sequence_id`, which means that the
 *   sequence recursively contains itself.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for internal buffers.
 */

YAML_DECLARE(int)
yaml_document_append_sequence_item(yaml_document_t *document,
        int sequence_id, int item_id);

/*
 * Add a pair of a key and a value to a MAPPING node.
 *
 * The order in which (key, value) pairs are added to a mapping coincides with
 * the order in which they are presented in the output stream.  Note that the
 * mapping key order is a presentation detail and should not used to convey any
 * information.  An ordered mapping could be represented as a sequence of
 * single-paired mappings.
 *
 * Arguments:
 *
 * - `document`: a document object.
 *
 * - `mapping_id`: the id of a mapping node; could be negative.  It is a
 *   fatal error if `mapping_id` does not refer to an existing mapping node.
 *
 * - `key_id`: the id of a key node; could be negative.  It is a fatal error
 *   if `key_id` does not refer to an existing node.
 *
 * - `value_id`: the id of a value node; could be negative.  It is a fatal
 *   error if `value_id` does not refer to an existing node.
 *
 * Returns: `1` on success, `0` on error.  The function may fail if it cannot
 * allocate memory for internal buffers.
 */

YAML_DECLARE(int)
yaml_document_append_mapping_pair(yaml_document_t *document,
        int mapping_id, int key_id, int value_id);

/******************************************************************************
 * Callback Definitions
 ******************************************************************************/

/*
 * The prototype of a read handler.
 *
 * The reader is called when the parser needs to read more bytes from the input
 * stream.  The reader is given a buffer to fill and should returns the number
 * of bytes read.  The reader should block if no data from the input stream is
 * available, but it should return immediately if it could produce least one
 * byte of the input stream.  If the reader reaches the stream end, it should
 * return immediately setting the number of bytes read to `0`.
 *
 * Arguments:
 *
 * - `data`: a pointer to an application data specified with
 *   `yaml_parser_set_reader()`.
 *
 * - `buffer`: a pointer to a buffer which should be filled with the bytes
 *   from the input stream.
 *
 * - `capacity`: the maximum capacity of the buffer.
 *
 * - `length`: a pointer to save the actual number of bytes read from the input
 *   stream; `length` equals `0` signifies that the reader reached the end of
 *   the stream.
 *
 * Return: on success, the reader should return `1`.  If the reader fails for
 * any reason, it should return `0`.  On the end of the input stream, the
 * reader should set `length` to `0` and return `1`.
 */

typedef int yaml_reader_t(void *data, unsigned char *buffer, size_t capacity,
        size_t *length);

/*
 * The prototype of a write handler.
 *
 * The writer is called when the emitter needs to flush the accumulated bytes
 * into the output stream.
 *
 * Arguments:
 *
 * - `data`: a pointer to an application data specified with
 *   `yaml_emitter_set_writer()`.
 *
 * - `buffer`: a pointer to a buffer with bytes to be written to the output
 *   stream.
 *
 * - `length`: the number of bytes to be written.
 *
 * Returns: on success, the writer should return `1`.  If the writer fails for
 * any reason, it should return `0`.
 */

typedef int yaml_writer_t(void *data, const unsigned char *buffer,
        size_t length);

/**
 * The prototype of a nonspecific tag resolver.
 *
 * The resolver is called when the parser encounters a node without an explicit
 * tag.  The resolver should determine the correct tag of the node from the
 * path to the node from the root node and, in case of the scalar node, the
 * node value.  The resolver is also called by the emitter to determine whether
 * the node tag could be omitted.
 *
 * Arguments:
 *
 * - `data`: a pointer to an application data specified with
 *   `yaml_parser_set_resolver()` or `yaml_emitter_set_resolver()`.
 *
 * - `node`: information about the new node.
 *
 * - `tag`: A pointer to save the guessed node tag.  The value returned by the
 *   resolved is immediately copied.
 *
 * Returns: on success, the resolver should return `1`.  If the resolver fails
 * for any reason, it should return `0`.
 */

typedef int yaml_resolver_t(void *data, yaml_incomplete_node_t *node,
        yaml_char_t **tag);

/******************************************************************************
 * Parser Definitions
 ******************************************************************************/

/*
 * An opaque definition of the parser object.
 *
 * A parser object is used to parse an input YAML stream producing a sequence
 * of YAML tokens, events or documents.
 */

typedef struct yaml_parser_s yaml_parser_t;

/*
 * Allocate a new parser object.
 *
 * An allocated parser object should be deleted with `yaml_parser_delete()`
 *
 * Returns: a new parser object or `NULL` on error.  The function may fail if
 * it cannot allocate memory for internal buffers.
 */

YAML_DECLARE(yaml_parser_t *)
yaml_parser_new(void);

/*
 * Deallocate a parser and free the internal parser data.
 * 
 * A parser object must be previously allocated with `yaml_parser_new()`.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 */

YAML_DECLARE(void)
yaml_parser_delete(yaml_parser_t *parser);

/*
 * Clear and reinitialize the internal state of the parser.
 *
 * This function could be used for cleaning up a parser object after an error
 * condition or after the end of the input stream is reached.  A cleaned parser
 * object may be reused to parse another YAML stream.  Note that all the parser
 * parameters including the read handler must be reset.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 */

YAML_DECLARE(void)
yaml_parser_clear(yaml_parser_t *parser);

/*
 * Get the parser error.
 *
 * Use this function to get a detailed error information after failure of one
 * of the following functions: `yaml_parser_parse_token()`,
 * `yaml_parser_parse_event()`, `yaml_parser_parse_document()`,
 * `yaml_parser_parse_single_document()`.
 *
 * The pointer returned by the function is only valid until the parser object
 * is not modified.  However the error object could be safely copied.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * Returns: a pointer to an error object.  The returned pointer is only valid
 * until the parser object is not modified or deleted.  However the error
 * object could be safely copied.
 */

YAML_DECLARE(yaml_error_t *)
yaml_parser_get_error(yaml_parser_t *parser);

/*
 * Set the parser to read the input stream from a character buffer.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `buffer`: a pointer to character buffer containing the input stream.  The
 *   buffer must be valid until the parser object is cleared or deleted.
 *
 * - `length`: the length of the buffer in bytes.
 */

YAML_DECLARE(void)
yaml_parser_set_string_reader(yaml_parser_t *parser,
        const unsigned char *buffer, size_t length);

/*
 * Set the parser to read the input stream from a file.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `file`: a pointer to an open file object.  The pointer must be valid until
 *   the parser object is cleared or deleted.
 */

YAML_DECLARE(void)
yaml_parser_set_file_reader(yaml_parser_t *parser, FILE *file);

/*
 * Set an input stream reader for a parser.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `reader`: a read handler.
 *
 * - `data`: application data for passing to the reader.
 */

YAML_DECLARE(void)
yaml_parser_set_reader(yaml_parser_t *parser,
        yaml_reader_t *reader, void *data);

/*
 * Set the standard nonspecific tag resolver for a parser.
 *
 * The standard resolver recognizes the following scalar tags: `!!null`,
 * `!!bool`, `!!str`, `!!int`, and `!!float`.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 */

YAML_DECLARE(void)
yaml_parser_set_standard_resolver(yaml_parser_t *parser);

/*
 * Set a nonspecific tag resolver for a parser.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `resolver`: a resolve handler.
 *
 * - `data`: application data for passing to the resolver.
 */

YAML_DECLARE(void)
yaml_parser_set_resolver(yaml_parser_t *parser,
        yaml_resolver_t *resolver, void *data);

/*
 * Set the input stream encoding.
 *
 * Typically the parser guesses the input stream encoding by the BOM mark.  If
 * the BOM mark is not present, the UTF-8 encoding is assumed.  An application
 * could override the detection mechanism by specifying the the encoding
 * explicitly.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `encoding`: the input stream encoding.
 */

YAML_DECLARE(void)
yaml_parser_set_encoding(yaml_parser_t *parser, yaml_encoding_t encoding);

/*
 * Parse the input stream and produce the next token.
 *
 * An application may call this function subsequently to produce a sequence of
 * tokens corresponding to the input stream.  The first token in the sequence
 * is STREAM-START and the last token is STREAM-END.  When the stream ends, the
 * parser produces empty tokens.
 *
 * An application must not alternate calls of the functions
 * `yaml_parser_parse_token()`, `yaml_parser_parse_event()`,
 * `yaml_parser_parse_document()` and `yaml_parser_parse_single_document()` on
 * the same parser object.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `token`: an empty token object to save the token produced by the parser.
 *   An application is responsible for clearing or deleting the produced token.
 *   If the parser fails or the stream end is reached, the object is kept
 *   empty.
 *
 * Returns: `1` on success, `0` on error.  If the function succeeds and the
 * stream end is not reached, the token data is saved to the given token
 * object.  If the function fails, the error details could be obtained with
 * `yaml_parser_get_error()`.  In case of error, the parser is non-functional
 * until it is cleared.
 */

YAML_DECLARE(int)
yaml_parser_parse_token(yaml_parser_t *parser, yaml_token_t *token);

/*
 * Parse the input stream and produce the next parsing event.
 *
 * An application may call this function subsequently to produce a sequence of
 * events corresponding to the input stream.  The produced events satisfy
 * the grammar:
 *
 *      stream ::= STREAM-START document* STREAM-END
 *      document ::= DOCUMENT-START node DOCUMENT-END
 *      node ::= ALIAS | SCALAR | sequence | mapping
 *      sequence ::= SEQUENCE-START node* SEQUENCE-END
 *      mapping ::= MAPPING-START (node node)* MAPPING-END
 *
 * When the stream ends, the parser produces empty tokens.
 *
 * An application must not alternate calls of the functions
 * `yaml_parser_parse_token()`, `yaml_parser_parse_event()`,
 * `yaml_parser_parse_document()` and `yaml_parser_parse_single_document()` on
 * the same parser object.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `event`: an empty event object to save the event produced by the parser.
 *   An application is responsible for clearing or deleting the produced event.
 *   Alternatively the produced event could be fed to the emitter.  If the
 *   parser fails or the stream end is reached, the object is kept empty.
 *
 * Returns: `1` on success, `0` on error.  If the function succeeds and the
 * stream end is not reached, the event data is saved to the given event
 * object.  If the function fails, the error details could be obtained with
 * `yaml_parser_get_error()`.  In case of error, the parser is non-functional
 * until it is cleared.
 */

YAML_DECLARE(int)
yaml_parser_parse_event(yaml_parser_t *parser, yaml_event_t *event);

/*
 * Parse the input stream and produce the next YAML document.
 *
 * An application may call this function subsequently to produce a sequence of
 * documents constituting the input stream.  When the stream ends, the parser
 * produces empty documents.
 *
 * An application must not alternate calls of the functions
 * `yaml_parser_parse_token()`, `yaml_parser_parse_event()`,
 * `yaml_parser_parse_document()` and `yaml_parser_parse_single_document()` on
 * the same parser object.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `document`: an empty document object to save the document produced by the
 *   parser.  An application is responsible for clearing or deleting the
 *   produced document.  Alternatively the produced document could be fed to
 *   the emitter.  If the parser fails or the stream end is reached, the object
 *   is kept empty.
 *
 * Returns: `1` on success, `0` on error.  If the function succeeds and the
 * stream end is not reached, the document data is saved to the given document
 * object.  If the function fails, the error details could be obtained with
 * `yaml_parser_get_error()`.  In case of error, the parser is non-functional
 * until it is cleared.
 */

YAML_DECLARE(int)
yaml_parser_parse_document(yaml_parser_t *parser, yaml_document_t *document);

/*
 * Parse the input stream containing a single YAML document.
 *
 * An application may call this function to ensure that the input stream contain
 * no more that one document.  If the stream is empty, the parser produces an
 * empty document.  If the stream contains a single document, the parser returns
 * it.  If the stream contains more than one document, the parser produces an
 * error.
 *
 * An application must not alternate calls of the functions
 * `yaml_parser_parse_token()`, `yaml_parser_parse_event()`,
 * `yaml_parser_parse_document()` and `yaml_parser_parse_single_document()` on
 * the same parser object.
 *
 * Arguments:
 *
 * - `parser`: a parser object.
 *
 * - `document`: an empty document object to save the document produced by the
 *   parser.  An application is responsible for clearing or deleting the
 *   produced document.  Alternatively the produced document could be fed to
 *   the emitter.  If the parser fails or the stream is empty, the object is
 *   kept empty.
 *
 * Returns: `1` on success, `0` on error.  If the function succeeds and the
 * stream is not empty, the document data is saved to the given document
 * object.  If the function fails, the error details could be obtained with
 * `yaml_parser_get_error()`.  In case of error, the parser is non-functional
 * until it is cleared.
 */

YAML_DECLARE(int)
yaml_parser_parse_single_document(yaml_parser_t *parser,
        yaml_document_t *document);

/******************************************************************************
 * Emitter Definitions
 ******************************************************************************/

/*
 * An opaque definition of the emitter object.
 *
 * An emitter object is used to emit YAML events or documents into an output
 * YAML stream.
 */

typedef struct yaml_emitter_s yaml_emitter_t;

/*
 * Allocate a new emitter object.
 *
 * An allocated emitter object should be deleted with `yaml_emitter_delete()`.
 *
 * Returns: a new emitter or `NULL` on error.  The function mail fail if it
 * cannot allocate memory for internal buffers.
 */

YAML_DECLARE(yaml_emitter_t *)
yaml_emitter_new(void);

/*
 * Deallocate an emitter and free the internal emitter data.
 * 
 * An emitter object must be previously allocated with `yaml_emitter_new()`.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_delete(yaml_emitter_t *emitter);

/*
 * Clear and reinitialize the internal state of the emitter.
 *
 * This function could be used for cleaning up an emitter object after an error
 * condition or after a complete YAML stream was produced.  A cleaned emitter
 * object may be reused to produce another YAML stream.  Note that all the
 * emitter parameters including the write handler must be reset.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_clear(yaml_emitter_t *emitter);

/*
 * Get the emitter error.
 *
 * Use this function to get a detailed error information after failure of one
 * of the following functions: `yaml_emitter_emit_event()`,
 * `yaml_emitter_open()`, `yaml_emitter_close()`,
 * `yaml_emitter_emit_document()`, `yaml_emitter_emit_single_document()`.
 *
 * The pointer returned by the function is only valid until the emitter object
 * is not modified.  However the error object could be safely copied.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * Returns: a pointer to an error object.  The returned pointer is only valid
 * until the emitter object is not modified or deleted.  However the error
 * object could be safely copied.
 */

YAML_DECLARE(yaml_error_t *)
yaml_emitter_get_error(yaml_emitter_t *emitter);

/*
 * Set the emitter to dump the generated YAML stream into a string buffer.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `buffer`: a pointer to a buffer to store the generated YAML stream.  The
 *   pointer must be valid until the emitter object is cleared or deleted.
 *
 * - `capacity`: the size of the buffer.  The emitter will fail if the buffer
 *   is smaller than required to hold the whole stream.
 *
 * - `length`: a pointer to save the length of the produced stream (in bytes).
 */

YAML_DECLARE(void)
yaml_emitter_set_string_writer(yaml_emitter_t *emitter,
        unsigned char *buffer, size_t capacity, size_t *length);

/*
 * Set the emitter to dump the generated YAML stream into a file.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `file`: a pointer to a file open for writing.  The pointer must be valid
 *   until the emitter object is cleared or deleted.
 */

YAML_DECLARE(void)
yaml_emitter_set_file_writer(yaml_emitter_t *emitter, FILE *file);

/*
 * Set the output stream writer for an emitter.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `writer`: a write handler.
 *
 * - `data`: application data for passing to the writer.
 */

YAML_DECLARE(void)
yaml_emitter_set_writer(yaml_emitter_t *emitter,
        yaml_writer_t *writer, void *data);

/*
 * Set the standard nonspecific tag resolver for an emitter.
 *
 * The standard resolver recognizes the following scalar tags: `!!null`,
 * `!!bool`, `!!str`, `!!int`, and `!!float`.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_set_standard_resolver(yaml_emitter_t *emitter);

/*
 * Set a nonspecific tag resolver for an emitter.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `resolver`: a resolve handler.
 *
 * - `data`: application data for passing to the resolver.
 */

YAML_DECLARE(void)
yaml_emitter_set_resolver(yaml_emitter_t *emitter,
        yaml_resolver_t *resolver, void *data);

/*
 * Set the output stream encoding.
 *
 * The emitter uses the UTF-8 encoding for the output stream unless another
 * encoding is specified explicitly.  The encoding could be specified using
 * this function or via the `encoding` parameter of the STREAM-START event.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `encoding`: the output stream encoding.
 */

YAML_DECLARE(void)
yaml_emitter_set_encoding(yaml_emitter_t *emitter, yaml_encoding_t encoding);

/*
 * Specify if the emitter should use the "canonical" output format.
 *
 * The "canonical" format uses the flow style for collections and the
 * double-quoted style for scalars.  Node tags are always dumped explicitly.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `is_canonical`: `1` to set the "canonical" format, `0` otherwise.
 */

YAML_DECLARE(void)
yaml_emitter_set_canonical(yaml_emitter_t *emitter, int is_canonical);

/*
 * Set the intendation increment.
 *
 * The default intendation increment is `2`.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `indent`: the indentation increment; a number between `2` and `9`.
 */

YAML_DECLARE(void)
yaml_emitter_set_indent(yaml_emitter_t *emitter, int indent);

/*
 * Set the preferred line width.
 *
 * The default preferred line width is `80`.  The given line width is only a
 * stylistic hint and could be violated by the emitter.  When the line width is
 * exceeded, the emitter seeks for a way to move to the next line.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `width`: the preferred line width; `-1` means unlimited.
 */

YAML_DECLARE(void)
yaml_emitter_set_width(yaml_emitter_t *emitter, int width);

/*
 * Specify if non-ASCII characters could be emitted unescaped.
 *
 * By default, the emitter always escapes non-ASCII characters using the `\u`
 * or `\U` format.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `is_unicode`: `1` if unescaped non-ASCII characters are allowed; `0`
 *   otherwise.
 */

YAML_DECLARE(void)
yaml_emitter_set_unicode(yaml_emitter_t *emitter, int is_unicode);

/*
 * Set the preferred line break.
 *
 * By default, the emitter uses the LN character for line breaks.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `line_break`: the preferred line break.
 */

YAML_DECLARE(void)
yaml_emitter_set_break(yaml_emitter_t *emitter, yaml_break_t line_break);

/*
 * Emit an event to the output YAML stream.
 *
 * An application needs to call this function subsequently to produce a whole
 * YAML stream.  The event sequence must satisfy the following grammar:
 *
 *      stream ::= STREAM-START document* STREAM-END
 *      document ::= DOCUMENT-START node DOCUMENT-END
 *      node ::= ALIAS | SCALAR | sequence | mapping
 *      sequence ::= SEQUENCE-START node* SEQUENCE-END
 *      mapping ::= MAPPING-START (node node)* MAPPING-END
 *
 * An application must not alternate the calls of the functions
 * `yaml_emitter_emit_event()`, `yaml_emitter_emit_document()` and
 * `yaml_emitter_emit_single_document()` on the same emitter object.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `event`: an event to emit.  The event must be previously initialized using
 *   either one of the functions `yaml_event_create_*()` or with
 *   `yaml_parser_parse_event()`.  The emitter takes the responsibility for the
 *   event data and clears the event, so that it becomes empty.  The event is
 *   cleared even if the function fails.
 *
 * Returns: `1` on success, `0` on error.  Note that the emitter may not
 * immediately dump the given event, so that the function could indicate
 * success on an errorneous event.  In this case, some of the next calls of the
 * function will generate an error.  If the function fails, the error details
 * could be obtained with `yaml_emitter_get_error()`.  In case of error, the
 * emitter is non-functional until it is cleared.
 */

YAML_DECLARE(int)
yaml_emitter_emit_event(yaml_emitter_t *emitter, yaml_event_t *event);

/*
 * Start a YAML stream.
 *
 * This function should be used in conjunction with
 * `yaml_emitter_emit_document()` and `yaml_emitter_end()`.  This function must
 * be called once before any documents are emitted.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * Returns: `1` on success, `0` on error.  If the function fails, the error
 * details could be obtained with `yaml_emitter_get_error()`.  In case of
 * error, the emitter is non-functional until it is cleared.
 */

YAML_DECLARE(int)
yaml_emitter_start(yaml_emitter_t *emitter);

/*
 * Finish a YAML stream.
 *
 * This function should be used in conjunction with `yaml_emitter_start()` and
 * `yaml_emitter_emit_document()`.  This function must be called once after all
 * documents are emitted.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * Returns: `1` on success, `0` on error.  If the function fails, the error
 * details could be obtained with `yaml_emitter_get_error()`.  In case of
 * error, the emitter is non-functional until it is cleared.
 */

YAML_DECLARE(int)
yaml_emitter_end(yaml_emitter_t *emitter);

/*
 * Emit a YAML document.
 *
 * Before emitting any documents, the function `yaml_emitter_start()` must be
 * called.  After all documents are emitted, the function `yaml_emitter_end()`
 * must be called.
 *
 * An application must not alternate the calls of the functions
 * `yaml_emitter_emit_event()`, `yaml_emitter_emit_document()` and
 * `yaml_emitter_emit_single_document()` on the same emitter object.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `document`: a document to emit.  The document must have a root node with
 *   all the other nodes reachable from it.  The document may be prepared using
 *   the functions `yaml_document_create()`, `yaml_document_add_*()` and
 *   `yaml_document_append_*()` or obtained via `yaml_parser_parse_document()`.
 *   The emitter takes the responsibility for the document content and clears
 *   the document, so that it becomes empty.  The document is cleared even if
 *   the function fails.
 *
 * Returns: `1` on success, `0` on error.  If the function fails, the error
 * details could be obtained with `yaml_emitter_get_error()`.  In case of
 * error, the emitter is non-functional until it is cleared.
 */

YAML_DECLARE(int)
yaml_emitter_emit_document(yaml_emitter_t *emitter, yaml_document_t *document);

/*
 * Emit a YAML stream consisting of a single document.
 *
 * This function is a shorthand of the calls:
 *
 *      yaml_emitter_start(emitter)
 *      yaml_emitter_emit_document(emitter, document)
 *      yaml_emitter_end(emitter)
 *
 * An application must not alternate the calls of the functions
 * `yaml_emitter_emit_event()`, `yaml_emitter_emit_document()` and
 * `yaml_emitter_emit_single_document()` on the same emitter object.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * - `document`: a document to emit.  The document must have a root node with
 *   all the other nodes reachable from it.  The document may be prepared using
 *   the functions `yaml_document_create()`, `yaml_document_add_*()` and
 *   `yaml_document_append_*()` or obtained via `yaml_parser_parse_document()`.
 *   The emitter takes the responsibility for the document content and clears
 *   the document, so that it becomes empty.  The document is cleared even if
 *   the function fails.
 *
 * Returns: `1` on success, `0` on error.  If the function fails, the error
 * details could be obtained with `yaml_emitter_get_error()`.  In case of
 * error, the emitter is non-functional until it is cleared.
 */

YAML_DECLARE(int)
yaml_emitter_emit_single_document(yaml_emitter_t *emitter,
        yaml_document_t *document);

/*
 * Flush the accumulated characters.
 *
 * This function flushes the accumulated characters from the internal emitter
 * buffer to the output stream.  Note that the buffer is flushed automatically
 * when a complete document has emitted or a stream has ended.
 *
 * Arguments:
 *
 * - `emitter`: an emitter object.
 *
 * Returns: `1` on success, `0` on error.  If the function fails, the error
 * details could be obtained with `yaml_emitter_get_error()`.  In case of
 * error, the emitter is non-functional until it is cleared.
 */

YAML_DECLARE(int)
yaml_emitter_flush(yaml_emitter_t *emitter);


#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_H */

