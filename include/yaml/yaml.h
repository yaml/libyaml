/**
 * @file yaml.h
 * @brief Public interface for libyaml.
 * 
 * Include the header file with the code:
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
#include <stdio.h>
#include <string.h>

/**
 * @defgroup version Version Information
 * @{
 */

/**
 * Get the library version as a string.
 *
 * @returns The function returns the pointer to a static string of the form
 * @c "X.Y.Z", where @c X is the major version number, @c Y is a minor version
 * number, and @c Z is the patch version number.
 */

const char *
yaml_get_version_string(void);

/**
 * Get the library version numbers.
 *
 * @param[out]  major   Major version number.
 * @param[out]  minor   Minor version number.
 * @param[out]  patch   Patch version number.
 */

void
yaml_get_version(int *major, int *minor, int *patch);

/** @} */

/**
 * @defgroup basic Basic Types
 * @{
 */

/** The character type. */
typedef unsigned char yaml_char_t;

/** The stream encoding. */
typedef enum {
    YAML_ANY_ENCODING,
    YAML_UTF8_ENCODING,
    YAML_UTF16LE_ENCODING,
    YAML_UTF16BE_ENCODING
} yaml_encoding_t;

/** Many bad things could happen with the parser and emitter. */
typedef enum {
    YAML_NO_ERROR,

    YAML_MEMORY_ERROR,

    YAML_READER_ERROR,
    YAML_SCANNER_ERROR,
    YAML_PARSER_ERROR,

    YAML_WRITER_ERROR,
    YAML_EMITTER_ERROR
} yaml_error_type_t;

/** @} */

/*

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

*/


/**
 * @defgroup parser Parser Definitions
 * @{
 */

/**
 * The prototype of a read handler.
 *
 * The read handler is called when the parser needs to read more bytes from the
 * source.  The handler should write not more than @a size bytes to the @a
 * buffer.  The number of written bytes should be set to the @a length variable.
 *
 * @param[in]   data        A pointer to an application data specified by
 *                          @c yaml_parser_set_read_handler.
 * @param[out]  buffer      The buffer to write the data from the source.
 * @param[in]   size        The size of the buffer.
 * @param[out]  size_read   The actual number of bytes read from the source.
 *
 * @returns On success, the handler should return @c 1.  If the handler failed,
 * the returned value should be @c 0.  On EOF, the handler should set the
 * @a length to @c 0 and return @c 1.
 */

typedef int yaml_read_handler_t(void *data, unsigned char *buffer, size_t size,
        size_t *size_read);

/**
 * This structure holds a string input specified by
 * @c yaml_parser_set_input_string.
 */

typedef struct {
    unsigned char *start;
    unsigned char *end;
    unsigned char *current;
} yaml_string_input_t;

/**
 * The parser structure.
 *
 * All members are internal.  Manage the structure using the @c yaml_parser_
 * family of functions.
 */

typedef struct {

    /**
     * @name Error handling
     * @{
     */

    /** Error type. */
    yaml_error_type_t error;

    /** Error description. */
    const char *problem;

    /** The byte about which the problem occured. */
    size_t problem_offset;

    /** The problematic value (@c -1 is none). */
    int problem_value;

    /**
     * @}
     */

    /**
     * @name Reader stuff
     * @{
     */

    /** Read handler. */
    yaml_read_handler_t *read_handler;

    /** A pointer for passing to the read handler. */
    void *read_handler_data;

    /** EOF flag */
    int eof;

    /** The pointer to the beginning of the working buffer. */
    yaml_char_t *buffer;

    /** The pointer to the end of the working buffer. */
    yaml_char_t *buffer_end;

    /** The pointer to the current character in the working buffer. */
    yaml_char_t *pointer;

    /** The number of unread characters in the working buffer. */
    size_t unread;

    /** The pointer to the beginning of the raw buffer. */
    unsigned char *raw_buffer;

    /** The pointer to the current character in the raw buffer. */
    unsigned char *raw_pointer;

    /** The number of unread bytes in the raw buffer. */
    size_t raw_unread;

    /** The input encoding. */
    yaml_encoding_t encoding;

    /** The offset of the current position (in bytes). */
    size_t offset;

    /** The index of the current position (in characters). */
    size_t index;

    /** The line of the current position (starting from @c 0). */
    size_t line;

    /** The column of the current position (starting from @c 0). */
    size_t column;

    /* String input structure. */
    yaml_string_input_t string_input;

    /**
     * @}
     */

} yaml_parser_t;

/**
 * Create a new parser.
 *
 * This function creates a new parser object.  An application is responsible
 * for destroying the object using the @c yaml_parser_delete function.
 *
 * @returns A new parser object; @c NULL on error.
 */

yaml_parser_t *
yaml_parser_new(void);

/**
 * Destroy a parser.
 *
 * @param[in]   parser  A parser object.
 */

void
yaml_parser_delete(yaml_parser_t *parser);

/**
 * Set a string input.
 *
 * Note that the @a input pointer must be valid while the @a parser object
 * exists.  The application is responsible for destroing @a input after
 * destroying the @a parser.
 *
 * @param[in]   parser  A parser object.
 * @param[in]   input   A source data.
 * @param[in]   length  The length of the source data in bytes.
 */

void
yaml_parser_set_input_string(yaml_parser_t *parser,
        unsigned char *input, size_t size);


/**
 * Set a file input.
 *
 * @a file should be a file object open for reading.  The application is
 * responsible for closing the @a file.
 *
 * @param[in]   parser  A parser object.
 * @param[in]   file    An open file.
 */

void
yaml_parser_set_input_file(yaml_parser_t *parser, FILE *file);

/**
 * Set a generic input handler.
 *
 * @param[in]   parser  A parser object.
 * @param[in]   handler A read handler.
 * @param[in]   data    Any application data for passing to the read handler.
 */

void
yaml_parser_set_input(yaml_parser_t *parser,
        yaml_read_handler_t *handler, void *data);

/**
 * Set the source encoding.
 *
 * @param[in]   encoding    The source encoding.
 */

void
yaml_parser_set_encoding(yaml_parser_t *parser, yaml_encoding_t encoding);

/** @} */

/*
typedef struct {
} yaml_emitter_t;
*/

/**
 * @defgroup internal Internal Definitions
 * @{
 */

/**
 * Allocate a dynamic memory block.
 *
 * @param[in]   size    Size of a memory block, \c 0 is valid.
 *
 * @returns @c yaml_malloc returns a pointer to a newly allocated memory block,
 * or @c NULL if it failed.
 */

void *
yaml_malloc(size_t size);

/**
 * Reallocate a dynamic memory block.
 *
 * @param[in]   ptr     A pointer to an existing memory block, \c NULL is
 *                      valid.
 * @param[in]   size    A size of a new block, \c 0 is valid.
 *
 * @returns @c yaml_realloc returns a pointer to a reallocated memory block,
 * or @c NULL if it failed.
 */

void *
yaml_realloc(void *ptr, size_t size);

/**
 * Free a dynamic memory block.
 *
 * @param[in]   ptr     A pointer to an existing memory block, \c NULL is
 *                      valid.
 */

void
yaml_free(void *ptr);

/** The size of the raw buffer. */

#define YAML_RAW_BUFFER_SIZE 16384

/**
 * The size of the buffer.
 *
 * We allocate enough space for decoding the whole raw buffer.
 */

#define YAML_BUFFER_SIZE    (YAML_RAW_BUFFER_SIZE*3)

/**
 * Ensure that the buffer contains at least @a length characters.
 *
 * @param[in]   parser  A parser object.
 * @param[in]   length  The number of characters in the buffer.
 *
 * @returns @c 1 on success, @c 0 on error.
 */

int
yaml_parser_update_buffer(yaml_parser_t *parser, size_t length);

/** @} */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_H */

