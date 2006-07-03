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
 * @defgroup export Export Definitions
 * @{
 */

/** The public API declaration. */

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

/** @} */

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

YAML_DECLARE(const char *)
yaml_get_version_string(void);

/**
 * Get the library version numbers.
 *
 * @param[out]  major   Major version number.
 * @param[out]  minor   Minor version number.
 * @param[out]  patch   Patch version number.
 */

YAML_DECLARE(void)
yaml_get_version(int *major, int *minor, int *patch);

/** @} */

/**
 * @defgroup basic Basic Types
 * @{
 */

/** The character type (UTF-8 octet). */
typedef unsigned char yaml_char_t;

/** The version directive data. */
typedef struct {
    /** The major version number. */
    int major;
    /** The minor version number. */
    int minor;
} yaml_version_directive_t;

/** The tag directive data. */
typedef struct {
    /** The tag handle. */
    yaml_char_t *handle;
    /** The tag prefix. */
    yaml_char_t *prefix;
} yaml_tag_directive_t;

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

/** The pointer position. */
typedef struct {
    /** The position index. */
    size_t index;

    /** The position line. */
    size_t line;

    /** The position column. */
    size_t column;
} yaml_mark_t;

/** @} */

/**
 * @defgroup styles Node Styles
 * @{
 */

/** Scalar styles. */
typedef enum {
    YAML_ANY_SCALAR_STYLE,

    YAML_PLAIN_SCALAR_STYLE,

    YAML_SINGLE_QUOTED_SCALAR_STYLE,
    YAML_DOUBLE_QUOTED_SCALAR_STYLE,

    YAML_LITERAL_SCALAR_STYLE,
    YAML_FOLDED_SCALAR_STYLE
} yaml_scalar_style_t;


/** Sequence styles. */
typedef enum {
    YAML_ANY_SEQUENCE_STYLE,

    YAML_BLOCK_SEQUENCE_STYLE,
    YAML_FLOW_SEQUENCE_STYLE
} yaml_sequence_style_t;

/** Mapping styles. */
typedef enum {
    YAML_ANY_MAPPING_STYLE,

    YAML_BLOCK_MAPPING_STYLE,
    YAML_FLOW_MAPPING_STYLE
} yaml_mapping_style_t;

/** @} */

/**
 * @defgroup tokens Tokens
 * @{
 */

/** Token types. */
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

/** The token structure. */
typedef struct {

    /** The token type. */
    yaml_token_type_t type;

    /** The token data. */
    union {

        /** The stream start (for @c YAML_STREAM_START_TOKEN). */
        struct {
            /** The stream encoding. */
            yaml_encoding_t encoding;
        } stream_start;

        /** The alias (for @c YAML_ALIAS_TOKEN). */
        struct {
            /** The alias value. */
            yaml_char_t *value;
        } alias;

        /** The anchor (for @c YAML_ANCHOR_TOKEN). */
        struct {
            /** The anchor value. */
            yaml_char_t *value;
        } anchor;

        /** The tag (for @c YAML_TAG_TOKEN). */
        struct {
            /** The tag handle. */
            yaml_char_t *handle;

            /** The tag suffix. */
            yaml_char_t *suffix;
        } tag;

        /** The scalar value (for @c YAML_SCALAR_TOKEN). */
        struct {

            /** The scalar value. */
            yaml_char_t *value;

            /** The length of the scalar value. */
            size_t length;

            /** The scalar style. */
            yaml_scalar_style_t style;
        } scalar;

        /** The version directive (for @c YAML_VERSION_DIRECTIVE_TOKEN). */
        struct {
            /** The major version number. */
            int major;

            /** The minor version number. */
            int minor;
        } version_directive;

        /** The tag directive (for @c YAML_TAG_DIRECTIVE_TOKEN). */
        struct {
            /** The tag handle. */
            yaml_char_t *handle;

            /** The tag prefix. */
            yaml_char_t *prefix;
        } tag_directive;
    } data;

    /** The beginning of the token. */
    yaml_mark_t start_mark;

    /** The end of the token. */
    yaml_mark_t end_mark;

} yaml_token_t;

/**
 * Create a new token without assigning any data.
 *
 * This function can be used for constructing indicator tokens:
 * @c YAML_DOCUMENT_START, @c YAML_DOCUMENT_END,
 * @c YAML_BLOCK_SEQUENCE_START_TOKEN, @c YAML_BLOCK_MAPPING_START_TOKEN,
 * @c YAML_BLOCK_END_TOKEN,
 * @c YAML_FLOW_SEQUENCE_START_TOKEN, @c YAML_FLOW_SEQUENCE_END_TOKEN,
 * @c YAML_FLOW_MAPPING_START_TOKEN, @c YAML_FLOW_MAPPING_END_TOKEN,
 * @c YAML_BLOCK_ENTRY_TOKEN, @c YAML_FLOW_ENTRY_TOKEN,
 * @c YAML_KEY_TOKEN, @c YAML_VALUE_TOKEN.
 *
 * @param[in]   type        The token type.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_token_new(yaml_token_type_t type,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_STREAM_START_TOKEN token with the specified encoding.
 *
 * @param[in]   encoding    The stream encoding.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_stream_start_token_new(yaml_encoding_t encoding,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_STREAM_END_TOKEN token.
 *
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_stream_end_token_new(yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_VERSION_DIRECTIVE_TOKEN token with the specified
 * version numbers.
 *
 * @param[in]   major       The major version number.
 * @param[in]   minor       The minor version number.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_version_directive_token_new(int major, int minor,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_TAG_DIRECTIVE_TOKEN token with the specified tag
 * handle and prefix.
 *
 * Note that the @a handle and the @a prefix pointers will be freed by
 * the token descructor.
 *
 * @param[in]   handle      The tag handle.
 * @param[in]   prefix      The tag prefix.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_tag_directive_token_new(yaml_char_t *handle, yaml_char_t *prefix,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_ALIAS_TOKEN token with the specified anchor.
 *
 * Note that the @a anchor pointer will be freed by the token descructor.
 *
 * @param[in]   anchor      The anchor.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_alias_token_new(yaml_char_t *anchor,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_ANCHOR_TOKEN token with the specified anchor.
 *
 * Note that the @a anchor pointer will be freed by the token descructor.
 *
 * @param[in]   anchor      The anchor.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_anchor_token_new(yaml_char_t *anchor,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_TAG_TOKEN token with the specified tag handle and
 * suffix.
 *
 * Note that the @a handle and the @a suffix pointers will be freed by
 * the token descructor.
 *
 * @param[in]   handle      The tag handle.
 * @param[in]   suffix      The tag suffix.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_tag_token_new(yaml_char_t *handle, yaml_char_t *suffix,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_SCALAR_TOKEN token with the specified scalar value,
 * length, and style.
 *
 * Note that the scalar value may contain the @c NUL character, therefore
 * the value length is also required.  The scalar value always ends with
 * @c NUL.
 *
 * Note that the @a value pointer will be freed by the token descructor.
 *
 * @param[in]   value       The scalar value.
 * @param[in]   length      The value length.
 * @param[in]   style       The scalar style.
 * @param[in]   start_mark  The beginning of the token.
 * @param[in]   end_mark    The end of the token.
 *
 * @returns A new token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_scalar_token_new(yaml_char_t *value, size_t length,
        yaml_scalar_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Destroy a token object.
 *
 * @param[in]   token   A token object.
 */

YAML_DECLARE(void)
yaml_token_delete(yaml_token_t *token);

/** @} */

/**
 * @defgroup events Events
 * @{
 */

/** Event types. */
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

/** The event structure. */
typedef struct {

    /** The event type. */
    yaml_event_type_t type;

    /** The event data. */
    union {
        
        /** The stream parameters (for @c YAML_STREAM_START_EVENT). */
        struct {
            /** The document encoding. */
            yaml_encoding_t encoding;
        } stream_start;

        /** The document parameters (for @c YAML_DOCUMENT_START_EVENT). */
        struct {
            /** The version directive. */
            yaml_version_directive_t *version_directive;
            /** The list of tag directives. */
            yaml_tag_directive_t **tag_directives;
            /** Is the document indicator implicit? */
            int implicit;
        } document_start;

        /** The document end parameters (for @c YAML_DOCUMENT_END_EVENT). */
        struct {
            /** Is the document end indicator implicit? */
            int implicit;
        } document_end;

        /** The alias parameters (for @c YAML_ALIAS_EVENT). */
        struct {
            /** The anchor. */
            yaml_char_t *anchor;
        } alias;

        /** The scalar parameters (for @c YAML_SCALAR_EVENT). */
        struct {
            /** The anchor. */
            yaml_char_t *anchor;
            /** The tag. */
            yaml_char_t *tag;
            /** The scalar value. */
            yaml_char_t *value;
            /** The length of the scalar value. */
            size_t length;
            /** Is the tag optional for the plain style? */
            int plain_implicit;
            /** Is the tag optional for any non-plain style? */
            int quoted_implicit;
            /** The scalar style. */
            yaml_scalar_style_t style;
        } scalar;

        /** The sequence parameters (for @c YAML_SEQUENCE_START_EVENT). */
        struct {
            /** The anchor. */
            yaml_char_t *anchor;
            /** The tag. */
            yaml_char_t *tag;
            /** Is the tag optional? */
            int implicit;
            /** The sequence style. */
            yaml_sequence_style_t style;
        } sequence_start;

        /** The mapping parameters (for @c YAML_MAPPING_START_EVENT). */
        struct {
            /** The anchor. */
            yaml_char_t *anchor;
            /** The tag. */
            yaml_char_t *tag;
            /** Is the tag optional? */
            int implicit;
            /** The mapping style. */
            yaml_mapping_style_t style;
        } mapping_start;

    } data;

    /** The beginning of the token. */
    yaml_mark_t start_mark;

    /** The end of the token. */
    yaml_mark_t end_mark;
} yaml_event_t;

/**
 * Create a new @c YAML_STREAM_START_EVENT event.
 *
 * @param[in]   encoding    The stream encoding.
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_stream_start_event_new(yaml_encoding_t encoding,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_STREAM_END_TOKEN event.
 *
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_stream_end_event_new(yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_DOCUMENT_START_EVENT event.
 *
 * @param[in]   version_directive   The version directive or @c NULL.
 * @param[in]   tag_directives      A list of tag directives or @c NULL.
 * @param[in]   implicit            Is the document indicator present?
 * @param[in]   start_mark          The beginning of the event.
 * @param[in]   end_mark            The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_document_start_event_new(yaml_version_directive_t *version_directive,
        yaml_tag_directive_t **tag_directives, int implicit,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_DOCUMENT_END_EVENT event.
 *
 * @param[in]   implicit    Is the document end indicator present?
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_document_end_event_new(int implicit,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_ALIAS_EVENT event.
 *
 * @param[in]   anchor      The anchor value.
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_alias_event_new(yaml_char_t *anchor,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_SCALAR_EVENT event.
 *
 * @param[in]   anchor          The anchor value or @c NULL.
 * @param[in]   tag             The tag value or @c NULL.
 * @param[in]   value           The scalar value.
 * @param[in]   length          The length of the scalar value.
 * @param[in]   plain_implicit  Is the tag optional for the plain style?
 * @param[in]   quoted_implicit Is the tag optional for any non-plain style?
 * @param[in]   style           The scalar style.
 * @param[in]   start_mark      The beginning of the event.
 * @param[in]   end_mark        The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_scalar_event_new(yaml_char_t *anchor, yaml_char_t *tag,
        yaml_char_t *value, size_t length,
        int plain_implicit, int quoted_implicit,
        yaml_scalar_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_SEQUENCE_START_EVENT event.
 *
 * @param[in]   anchor      The anchor value or @c NULL.
 * @param[in]   tag         The tag value or @c NULL.
 * @param[in]   implicit    Is the tag optional?
 * @param[in]   style       The sequence style.
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_sequence_start_new(yaml_char_t *anchor, yaml_char_t *tag,
        int implicit, yaml_sequence_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_SEQUENCE_END_EVENT event.
 *
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_sequence_end_new(yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_MAPPING_START_EVENT event.
 *
 * @param[in]   anchor      The anchor value or @c NULL.
 * @param[in]   tag         The tag value or @c NULL.
 * @param[in]   implicit    Is the tag optional?
 * @param[in]   style       The mapping style.
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_mapping_start_new(yaml_char_t *anchor, yaml_char_t *tag,
        int implicit, yaml_mapping_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Create a new @c YAML_MAPPING_END_EVENT event.
 *
 * @param[in]   start_mark  The beginning of the event.
 * @param[in]   end_mark    The end of the event.
 *
 * @returns A new event object, or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_mapping_end_new(yaml_mark_t start_mark, yaml_mark_t end_mark);

/**
 * Destroy an event object.
 *
 * @param[in]   event   An event object.
 */

YAML_DECLARE(void)
yaml_event_delete(yaml_event_t *event);

/** @} */

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
    /** The string start pointer. */
    unsigned char *start;

    /** The string end pointer. */
    unsigned char *end;

    /** The string current position. */
    unsigned char *current;
} yaml_string_input_t;

/**
 * This structure holds information about a potential simple key.
 */

typedef struct {
    /** Is a simple key required? */
    int required;

    /** The number of the token. */
    size_t token_number;

    /** The position index. */
    size_t index;

    /** The position line. */
    size_t line;

    /** The position column. */
    size_t column;

    /** The position mark. */
    yaml_mark_t mark;
} yaml_simple_key_t;

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

    /** The problem position. */
    yaml_mark_t problem_mark;

    /** The error context. */
    const char *context;

    /** The context position. */
    yaml_mark_t context_mark;

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

    /**
     * @name Scanner stuff
     * @{
     */

    /** Have we started to scan the input stream? */
    int stream_start_produced;

    /** Have we reached the end of the input stream? */
    int stream_end_produced;

    /** The number of unclosed '[' and '{' indicators. */
    int flow_level;

    /** The tokens queue, which contains the current produced tokens. */
    yaml_token_t **tokens;

    /** The size of the tokens queue. */
    size_t tokens_size;

    /** The head of the tokens queue. */
    size_t tokens_head;

    /** The tail of the tokens queue. */
    size_t tokens_tail;

    /** The number of tokens fetched from the tokens queue. */
    size_t tokens_parsed;

    /** The stack of indentation levels. */
    int *indents;

    /** The size of the indents stack. */
    size_t indents_size;

    /** The number of items in the indents stack. */
    size_t indents_length;

    /** The current indentation level. */
    int indent;

    /** May a simple key occur at the current position? */
    int simple_key_allowed;

    /** The stack of potential simple keys. */
    yaml_simple_key_t **simple_keys;

    /** The size of the simple keys stack. */
    size_t simple_keys_size;

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

YAML_DECLARE(yaml_parser_t *)
yaml_parser_new(void);

/**
 * Destroy a parser.
 *
 * @param[in]   parser  A parser object.
 */

YAML_DECLARE(void)
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
 * @param[in]   size    The length of the source data in bytes.
 */

YAML_DECLARE(void)
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

YAML_DECLARE(void)
yaml_parser_set_input_file(yaml_parser_t *parser, FILE *file);

/**
 * Set a generic input handler.
 *
 * @param[in]   parser  A parser object.
 * @param[in]   handler A read handler.
 * @param[in]   data    Any application data for passing to the read handler.
 */

YAML_DECLARE(void)
yaml_parser_set_input(yaml_parser_t *parser,
        yaml_read_handler_t *handler, void *data);

/**
 * Set the source encoding.
 *
 * @param[in]   parser      A parser object.
 * @param[in]   encoding    The source encoding.
 */

YAML_DECLARE(void)
yaml_parser_set_encoding(yaml_parser_t *parser, yaml_encoding_t encoding);

/**
 * Get the next token.
 *
 * The token is removed from the internal token queue and the application is
 * responsible for destroing the token object.
 *
 * @param[in]   parser      A parser object.
 *
 * @returns A token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_parser_get_token(yaml_parser_t *parser);

/**
 * Peek the next token.
 *
 * The token is not removed from the internal token queue and will be returned
 * again on a subsequent call of @c yaml_parser_get_token or
 * @c yaml_parser_peek_token. The application should not destroy the token
 * object.
 *
 * @param[in]   parser      A parser object.
 *
 * @returns A token object, or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_parser_peek_token(yaml_parser_t *parser);

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

YAML_DECLARE(void *)
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

YAML_DECLARE(void *)
yaml_realloc(void *ptr, size_t size);

/**
 * Free a dynamic memory block.
 *
 * @param[in]   ptr     A pointer to an existing memory block, \c NULL is
 *                      valid.
 */

YAML_DECLARE(void)
yaml_free(void *ptr);

/** The initial size for various buffers. */

#define YAML_DEFAULT_SIZE   16

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

YAML_DECLARE(int)
yaml_parser_update_buffer(yaml_parser_t *parser, size_t length);

/** @} */


#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_H */

