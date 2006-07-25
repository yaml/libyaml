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

/** Line break types. */

typedef enum {
    YAML_ANY_BREAK,
    YAML_CR_BREAK,
    YAML_LN_BREAK,
    YAML_CRLN_BREAK
} yaml_break_t;

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
    YAML_FLOW_MAPPING_STYLE,
    YAML_FLOW_SET_MAPPING_STYLE
} yaml_mapping_style_t;

/** @} */

/**
 * @defgroup tokens Tokens
 * @{
 */

/** Token types. */
typedef enum {
    YAML_NO_TOKEN,

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
 * Free any memory allocated for a token object.
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
    YAML_NO_EVENT,

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
            struct {
                /** The beginning of the tag directives list. */
                yaml_tag_directive_t *start;
                /** The end of the tag directives list. */
                yaml_tag_directive_t *end;
            } tag_directives;

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
 * Free any memory allocated for an event object.
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
 * @a size_read to @c 0 and return @c 1.
 */

typedef int yaml_read_handler_t(void *data, unsigned char *buffer, size_t size,
        size_t *size_read);

/**
 * This structure holds information about a potential simple key.
 */

typedef struct {
    /** Is a simple key possible? */
    int possible;

    /** Is a simple key required? */
    int required;

    /** The number of the token. */
    size_t token_number;

    /** The position mark. */
    yaml_mark_t mark;
} yaml_simple_key_t;

/**
 * The states of the parser.
 */
typedef enum {
    YAML_PARSE_STREAM_START_STATE,
    YAML_PARSE_IMPLICIT_DOCUMENT_START_STATE,
    YAML_PARSE_DOCUMENT_START_STATE,
    YAML_PARSE_DOCUMENT_CONTENT_STATE,
    YAML_PARSE_DOCUMENT_END_STATE,
    YAML_PARSE_BLOCK_NODE_STATE,
    YAML_PARSE_BLOCK_NODE_OR_INDENTLESS_SEQUENCE_STATE,
    YAML_PARSE_FLOW_NODE_STATE,
    YAML_PARSE_BLOCK_SEQUENCE_FIRST_ENTRY_STATE,
    YAML_PARSE_BLOCK_SEQUENCE_ENTRY_STATE,
    YAML_PARSE_INDENTLESS_SEQUENCE_ENTRY_STATE,
    YAML_PARSE_BLOCK_MAPPING_FIRST_KEY_STATE,
    YAML_PARSE_BLOCK_MAPPING_KEY_STATE,
    YAML_PARSE_BLOCK_MAPPING_VALUE_STATE,
    YAML_PARSE_FLOW_SEQUENCE_FIRST_ENTRY_STATE,
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_STATE,
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_KEY_STATE,
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_VALUE_STATE,
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_END_STATE,
    YAML_PARSE_FLOW_MAPPING_FIRST_KEY_STATE,
    YAML_PARSE_FLOW_MAPPING_KEY_STATE,
    YAML_PARSE_FLOW_MAPPING_VALUE_STATE,
    YAML_PARSE_FLOW_MAPPING_EMPTY_VALUE_STATE,
    YAML_PARSE_END_STATE
} yaml_parser_state_t;

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

    /** Standard (string or file) input data. */
    union {
        /** String input data. */
        struct {
            /** The string start pointer. */
            unsigned char *start;
            /** The string end pointer. */
            unsigned char *end;
            /** The string current position. */
            unsigned char *current;
        } string;

        /** File input data. */
        FILE *file;
    } input;

    /** EOF flag */
    int eof;

    /** The working buffer. */
    struct {
        /** The beginning of the buffer. */
        yaml_char_t *start;
        /** The end of the buffer. */
        yaml_char_t *end;
        /** The current position of the buffer. */
        yaml_char_t *pointer;
        /** The last filled position of the buffer. */
        yaml_char_t *last;
    } buffer;

    /* The number of unread characters in the buffer. */
    size_t unread;

    /** The raw buffer. */
    struct {
        /** The beginning of the buffer. */
        unsigned char *start;
        /** The end of the buffer. */
        unsigned char *end;
        /** The current position of the buffer. */
        unsigned char *pointer;
        /** The last filled position of the buffer. */
        unsigned char *last;
    } raw_buffer;

    /** The input encoding. */
    yaml_encoding_t encoding;

    /** The offset of the current position (in bytes). */
    size_t offset;

    /** The mark of the current position. */
    yaml_mark_t mark;

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

    /** The tokens queue. */
    struct {
        /** The beginning of the tokens queue. */
        yaml_token_t *start;
        /** The end of the tokens queue. */
        yaml_token_t *end;
        /** The head of the tokens queue. */
        yaml_token_t *head;
        /** The tail of the tokens queue. */
        yaml_token_t *tail;
    } tokens;

    /** The number of tokens fetched from the queue. */
    size_t tokens_parsed;

    /* Does the tokens queue contain a token ready for dequeueing. */
    int token_available;

    /** The indentation levels stack. */
    struct {
        /** The beginning of the stack. */
        int *start;
        /** The end of the stack. */
        int *end;
        /** The top of the stack. */
        int *top;
    } indents;

    /** The current indentation level. */
    int indent;

    /** May a simple key occur at the current position? */
    int simple_key_allowed;

    /** The stack of simple keys. */
    struct {
        /** The beginning of the stack. */
        yaml_simple_key_t *start;
        /** The end of the stack. */
        yaml_simple_key_t *end;
        /** The top of the stack. */
        yaml_simple_key_t *top;
    } simple_keys;

    /**
     * @}
     */

    /**
     * @name Parser stuff
     * @{
     */

    /** The parser states stack. */
    struct {
        /** The beginning of the stack. */
        yaml_parser_state_t *start;
        /** The end of the stack. */
        yaml_parser_state_t *end;
        /** The top of the stack. */
        yaml_parser_state_t *top;
    } states;

    /** The current parser state. */
    yaml_parser_state_t state;

    /** The stack of marks. */
    struct {
        /** The beginning of the stack. */
        yaml_mark_t *start;
        /** The end of the stack. */
        yaml_mark_t *end;
        /** The top of the stack. */
        yaml_mark_t *top;
    } marks;

    /** The list of TAG directives. */
    struct {
        /** The beginning of the list. */
        yaml_tag_directive_t *start;
        /** The end of the list. */
        yaml_tag_directive_t *end;
        /** The top of the list. */
        yaml_tag_directive_t *top;
    } tag_directives;

    /**
     * @}
     */

} yaml_parser_t;

/**
 * Initialize a parser.
 *
 * This function creates a new parser object.  An application is responsible
 * for destroying the object using the @c yaml_parser_delete function.
 *
 * @param[in]   parser  An empty parser object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_parser_initialize(yaml_parser_t *parser);

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
 * Scan the input stream and produce the next token.
 *
 * Call the function subsequently to produce a sequence of tokens corresponding
 * to the input stream.  The initial token has the type
 * @c YAML_STREAM_START_TOKEN while the ending token has the type
 * @c YAML_STREAM_END_TOKEN.
 *
 * An application is responsible for freeing any buffers associated with the
 * produced token object using the @c yaml_token_delete function.
 *
 * An application must not alternate the calls of @c yaml_parser_scan with the
 * calls of @c yaml_parser_parse. Doing this will break the parser.
 *
 * @param[in]   parser      A parser object.
 * @param[in]   token       An empty token object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_parser_scan(yaml_parser_t *parser, yaml_token_t *token);

/**
 * Parse the input stream and produce the next parsing event.
 *
 * Call the function subsequently to produce a sequence of events corresponding
 * to the input stream.  The initial event has the type
 * @c YAML_STREAM_START_EVENT while the ending event has the type
 * @c YAML_STREAM_END_EVENT.
 *
 * An application is responsible for freeing any buffers associated with the
 * produced event object using the @c yaml_event_delete function.
 *
 * An application must not alternate the calls of @c yaml_parser_scan with the
 * calls of @c yaml_parser_parse. Doing this will break the parser.
 *
 * @param[in]   parser      A parser object.
 * @param[in]   event       An empty event object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_parser_parse(yaml_parser_t *parser, yaml_event_t *event);

/** @} */

/**
 * @defgroup emitter Emitter Definitions
 * @{
 */

/**
 * The prototype of a write handler.
 *
 * The write handler is called when the emitter needs to flush the accumulated
 * characters to the output.  The handler should write @a size bytes of the
 * @a buffer to the output.
 *
 * @param[in]   data        A pointer to an application data specified by
 *                          @c yaml_emitter_set_write_handler.
 * @param[out]  buffer      The buffer with bytes to be written.
 * @param[in]   size        The size of the buffer.
 *
 * @returns On success, the handler should return @c 1.  If the handler failed,
 * the returned value should be @c 0.
 */

typedef int yaml_write_handler_t(void *data, unsigned char *buffer, size_t size);

/** The emitter states. */
typedef enum {
    YAML_EMIT_STREAM_START_STATE,
    YAML_EMIT_FIRST_DOCUMENT_START_STATE,
    YAML_EMIT_DOCUMENT_START_STATE,
    YAML_EMIT_DOCUMENT_CONTENT_STATE,
    YAML_EMIT_DOCUMENT_END_STATE,
    YAML_EMIT_FLOW_SEQUENCE_FIRST_ITEM_STATE,
    YAML_EMIT_FLOW_SEQUENCE_ITEM_STATE,
    YAML_EMIT_FLOW_MAPPING_FIRST_KEY_STATE,
    YAML_EMIT_FLOW_MAPPING_KEY_STATE,
    YAML_EMIT_FLOW_MAPPING_SIMPLE_VALUE_STATE,
    YAML_EMIT_FLOW_MAPPING_VALUE_STATE,
    YAML_EMIT_BLOCK_SEQUENCE_FIRST_ITEM_STATE,
    YAML_EMIT_BLOCK_SEQUENCE_ITEM_STATE,
    YAML_EMIT_BLOCK_MAPPING_FIRST_KEY_STATE,
    YAML_EMIT_BLOCK_MAPPING_KEY_STATE,
    YAML_EMIT_BLOCK_MAPPING_SIMPLE_VALUE_STATE,
    YAML_EMIT_BLOCK_MAPPING_VALUE_STATE
} yaml_emitter_state_t;

/**
 * The emitter structure.
 *
 * All members are internal.  Manage the structure using the @c yaml_emitter_
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

    /**
     * @}
     */

    /**
     * @name Writer stuff
     * @{
     */

    /** Write handler. */
    yaml_write_handler_t *write_handler;

    /** A pointer for passing to the white handler. */
    void *write_handler_data;

    /** Standard (string or file) output data. */
    union {
        /** String output data. */
        struct {
            /** The buffer pointer. */
            unsigned char *buffer;
            /** The buffer size. */
            size_t size;
            /** The number of written bytes. */
            size_t *size_written;
        } string;

        /** File output data. */
        FILE *file;
    } output;

    /** The working buffer. */
    struct {
        /** The beginning of the buffer. */
        yaml_char_t *start;
        /** The end of the buffer. */
        yaml_char_t *end;
        /** The current position of the buffer. */
        yaml_char_t *pointer;
        /** The last filled position of the buffer. */
        yaml_char_t *last;
    } buffer;

    /** The raw buffer. */
    struct {
        /** The beginning of the buffer. */
        unsigned char *start;
        /** The end of the buffer. */
        unsigned char *end;
        /** The current position of the buffer. */
        unsigned char *pointer;
        /** The last filled position of the buffer. */
        unsigned char *last;
    } raw_buffer;

    /** The stream encoding. */
    yaml_encoding_t encoding;

    /**
     * @}
     */

    /**
     * @name Emitter stuff
     * @{
     */

    /** If the output is in the canonical style? */
    int canonical;
    /** The number of indentation spaces. */
    int best_indent;
    /** The preferred width of the output lines. */
    int best_width;
    /** Allow unescaped non-ASCII characters? */
    int unicode;
    /** The preferred line break. */
    yaml_break_t line_break;

    /** The stack of states. */
    struct {
        /** The beginning of the stack. */
        yaml_emitter_state_t *start;
        /** The end of the stack. */
        yaml_emitter_state_t *end;
        /** The top of the stack. */
        yaml_emitter_state_t *top;
    } states;

    /** The current emitter state. */
    yaml_emitter_state_t state;

    /** The event queue. */
    struct {
        /** The beginning of the event queue. */
        yaml_event_t *start;
        /** The end of the event queue. */
        yaml_event_t *end;
        /** The head of the event queue. */
        yaml_event_t *head;
        /** The tail of the event queue. */
        yaml_event_t *tail;
    } events;

    /** The current event. */
    yaml_event_t event;

    /** The stack of indentation levels. */
    struct {
        /** The beginning of the stack. */
        int *start;
        /** The end of the stack. */
        int *end;
        /** The top of the stack. */
        int *top;
    } indents;

    /** The list of tag directives. */
    struct {
        /** The beginning of the list. */
        yaml_tag_directive_t *start;
        /** The end of the list. */
        yaml_tag_directive_t *end;
        /** The top of the list. */
        yaml_tag_directive_t *top;
    } tag_directives;

    /** The current indentation level. */
    int indent;

    /** The current flow level. */
    int flow_level;

    /** Is it the document root context? */
    int root_context;
    /** Is it a sequence context? */
    int sequence_context;
    /** Is it a mapping context? */
    int mapping_context;
    /** Is it a simple mapping key context? */
    int simple_key_context;

    /** The current line. */
    int line;
    /** The current column. */
    int column;
    /** If the last character was a whitespace? */
    int whitespace;
    /** If the last character was an indentation character (' ', '-', '?', ':')? */
    int indention;

    /**
     * @}
     */

} yaml_emitter_t;

/**
 * Initialize an emitter.
 *
 * This function creates a new emitter object.  An application is responsible
 * for destroying the object using the @c yaml_emitter_delete function.
 *
 * @param[in]   emitter An empty parser object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_initialize(yaml_emitter_t *emitter);

/**
 * Destroy an emitter.
 *
 * @param[in]   emitter An emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_delete(yaml_emitter_t *emitter);

/**
 * Set a string output.
 *
 * The emitter will write the output characters to the @a output buffer of the
 * size @a size.  The emitter will set @a size_written to the number of written
 * bytes.  If the buffer is smaller than required, the emitter produces the
 * YAML_WRITE_ERROR error.
 *
 * @param[in]   emitter         An emitter object.
 * @param[in]   output          An output buffer.
 * @param[in]   size            The buffer size.
 * @param[in]   size_written    The pointer to save the number of written bytes.
 */

YAML_DECLARE(void)
yaml_emitter_set_output_string(yaml_emitter_t *emitter,
        unsigned char *output, size_t size, size_t *size_written);

/**
 * Set a file output.
 *
 * @a file should be a file object open for writing.  The application is
 * responsible for closing the @a file.
 *
 * @param[in]   emitter An emitter object.
 * @param[in]   file    An open file.
 */

YAML_DECLARE(void)
yaml_emitter_set_output_file(yaml_emitter_t *emitter, FILE *file);

/**
 * Set a generic output handler.
 *
 * @param[in]   emitter An emitter object.
 * @param[in]   handler A write handler.
 * @param[in]   data    Any application data for passing to the write handler.
 */

YAML_DECLARE(void)
yaml_emitter_set_output(yaml_emitter_t *emitter,
        yaml_write_handler_t *handler, void *data);

/**
 * Set the output encoding.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   encoding    The output encoding.
 */

YAML_DECLARE(void)
yaml_emitter_set_encoding(yaml_emitter_t *emitter, yaml_encoding_t encoding);

/**
 * Set if the output should be in the "canonical" format as in the YAML
 * specification.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   canonical   If the output is canonical.
 */

YAML_DECLARE(void)
yaml_emitter_set_canonical(yaml_emitter_t *emitter, int canonical);

/**
 * Set the intendation increment.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   indent      The indentation increment (1 < . < 10).
 */

YAML_DECLARE(void)
yaml_emitter_set_indent(yaml_emitter_t *emitter, int indent);

/**
 * Set the preferred line width. @c -1 means unlimited.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   width       The preferred line width.
 */

YAML_DECLARE(void)
yaml_emitter_set_width(yaml_emitter_t *emitter, int width);

/**
 * Set if unescaped non-ASCII characters are allowed.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   unicode     If unescaped Unicode characters are allowed.
 */

YAML_DECLARE(void)
yaml_emitter_set_unicode(yaml_emitter_t *emitter, int unicode);

/**
 * Set the preferred line break.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   line_break  The preferred line break.
 */

YAML_DECLARE(void)
yaml_emitter_set_break(yaml_emitter_t *emitter, yaml_break_t line_break);

/**
 * Emit an event.
 *
 * The event object may be generated using the @c yaml_parser_parse function.
 * The emitter will destroy the event object if the function succeeds.  If the
 * function fails, the application is responsible for destroing the event
 * object.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   event       An event object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit(yaml_emitter_t *emitter, yaml_event_t *event);

/**
 * Emit the STREAM-START event.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   encoding    The stream encoding.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_stream_start(yaml_emitter_t *emitter,
        yaml_encoding_t encoding);

/**
 * Emit the STREAM-END event.
 *
 * @param[in]   emitter     An emitter object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_stream_end(yaml_emitter_t *emitter);

/**
 * Emit the DOCUMENT-START event.
 *
 * The @a implicit argument is considered as a stylistic parameter and may be
 * ignored by the emitter.
 *
 * @param[in]   emitter                 An emitter object.
 * @param[in]   version_directive       The %YAML directive value or @c NULL.
 * @param[in]   tag_directives_start    The beginning of the %TAG directives list.
 * @param[in]   tag_directives_end      The end of the %TAG directives list.
 * @param[in]   implicit                If the document start indicator is implicit.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_document_start(yaml_emitter_t *emitter,
        yaml_version_directive_t *version_directive,
        yaml_tag_directive_t *tag_directives_start,
        yaml_tag_directive_t *tag_directives_end,
        int implicit);

/**
 * Emit the DOCUMENT-END event.
 *
 * The @a implicit argument is considered as a stylistic parameter and may be
 * ignored by the emitter.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   implicit    If the document end indicator is implicit.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_document_end(yaml_emitter_t *emitter, int implicit);

/**
 * Emit an ALIAS event.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   anchor      The anchor value.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_alias(yaml_emitter_t *emitter, yaml_char_t *anchor);

/**
 * Emit a SCALAR event.
 *
 * The @a style argument may be ignored by the emitter.
 *
 * Either the @a tag attribute or one of the @a plain_implicit and
 * @a quoted_implicit flags must be set.
 *
 * @param[in]   emitter         An emitter object.
 * @param[in]   anchor          The scalar anchor or @c NULL.
 * @param[in]   tag             The scalar tag or @c NULL.
 * @param[in]   value           The scalar value.
 * @param[in]   length          The length of the scalar value.
 * @param[in]   plain_implicit  If the tag may be omitted for the plain style.
 * @param[in]   quoted_implicit If the tag may be omitted for any non-plain style.
 * @param[in]   style           The scalar style.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_scalar(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag,
        yaml_char_t *value, size_t length,
        int plain_implicit, int quoted_implicit,
        yaml_scalar_style_t style);

/**
 * Emit a SEQUENCE-START event.
 *
 * The @a style argument may be ignored by the emitter.
 *
 * Either the @a tag attribute or the @a implicit flag must be set.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   anchor      The sequence anchor or @c NULL.
 * @param[in]   tag         The sequence tag or @c NULL.
 * @param[in]   implicit    If the tag may be omitted.
 * @param[in]   style       The sequence style.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_sequence_start(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag, int implicit,
        yaml_sequence_style_t style);

/**
 * Emit a SEQUENCE-END event.
 *
 * @param[in]   emitter     An emitter object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_sequence_end(yaml_emitter_t *emitter);

/**
 * Emit a MAPPING-START event.
 *
 * The @a style argument may be ignored by the emitter.
 *
 * Either the @a tag attribute or the @a implicit flag must be set.
 *
 * @param[in]   emitter     An emitter object.
 * @param[in]   anchor      The mapping anchor or @c NULL.
 * @param[in]   tag         The mapping tag or @c NULL.
 * @param[in]   implicit    If the tag may be omitted.
 * @param[in]   style       The mapping style.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_mapping_start(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag, int implicit,
        yaml_mapping_style_t style);

/**
 * Emit a MAPPING-END event.
 *
 * @param[in]   emitter     An emitter object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_mapping_end(yaml_emitter_t *emitter);

/**
 * Flush the accumulated characters to the output.
 *
 * @param[in]   emitter     An emitter object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_flush(yaml_emitter_t *emitter);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* #ifndef YAML_H */

