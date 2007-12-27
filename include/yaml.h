/**
 * @file yaml.h
 * @brief Public interface for libyaml.
 * 
 * Include the header file with the code:
 * @code
 * #include <yaml.h>
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

/** The major version number. */
#define YAML_VERSION_MAJOR  0

/** The minor version number. */
#define YAML_VERSION_MINOR  2

/** The patch version number. */
#define YAML_VERSION_PATCH  0

/** The version string generator macro. */
#define YAML_VERSION_STRING_GENERATOR(major,minor,patch)                        \
    (#major "." #minor "." #patch)

/** The version string. */
#define YAML_VERSION_STRING                                                     \
    YAML_VERSION_STRING_GENERATOR(YAML_VERSION_MAJOR,YAML_VERSION_MINOR,YAML_VERSION_PATCH)

/**
 * Get the library version numbers at runtime.
 *
 * @param[out]      major   Major version number.
 * @param[out]      minor   Minor version number.
 * @param[out]      patch   Patch version number.
 */

YAML_DECLARE(void)
yaml_get_version(int *major, int *minor, int *patch);

/**
 * Get the library version as a string at runtime.
 *
 * @returns The function returns the pointer to a static string of the form
 * @c "X.Y.Z", where @c X is the major version number, @c Y is the minor version
 * number, and @c Z is the patch version number.
 */

YAML_DECLARE(const char *)
yaml_get_version_string(void);

/** @} */

/**
 * @defgroup styles Error Handling
 * @{
 */

/** Many bad things could happen with the parser and the emitter. */
typedef enum yaml_error_type_e {
    /** No error is produced. */
    YAML_NO_ERROR,

    /** Cannot allocate or reallocate a block of memory. */
    YAML_MEMORY_ERROR,

    /** Cannot read from the input stream. */
    YAML_READER_ERROR,
    /** Cannot decode the input stream. */
    YAML_DECODER_ERROR,
    /** Cannot scan a YAML token. */
    YAML_SCANNER_ERROR,
    /** Cannot parse a YAML production. */
    YAML_PARSER_ERROR,
    /** Cannot compose a YAML document. */
    YAML_COMPOSER_ERROR,

    /** Cannot write into the output stream. */
    YAML_WRITER_ERROR,
    /** Cannot emit a YAML event. */
    YAML_EMITTER_ERROR,
    /** Cannot serialize a YAML document. */
    YAML_SERIALIZER_ERROR,

    /** Cannot resolve an implicit tag. */
    YAML_RESOLVER_ERROR
} yaml_error_type_t;

/** The pointer position. */
typedef struct yaml_mark_s {
    /** The character number (starting from zero). */
    size_t index;

    /** The mark line (starting from zero). */
    size_t line;

    /** The mark column (starting from zero). */
    size_t column;
} yaml_mark_t;

/** The error details. */
typedef struct yaml_error_s {

    /** The error type. */
    yaml_error_type_t type;

    /** The detailed error information. */
    union {

        /**
         * A problem while reading the stream (@c YAML_READER_ERROR or
         * @c YAML_DECODER_ERROR).
         */
        struct {
            /** The problem description. */
            const char *problem;
            /** The stream offset. */
            size_t offset;
            /** The problematic octet or character (@c -1 if not applicable). */
            int value;
        } reading;

        /**
         * A problem while loading the stream (@c YAML_SCANNER_ERROR,
         * @c YAML_PARSER_ERROR, or @c YAML_COMPOSER_ERROR).
         */
        struct {
            /** The context in which the problem occured
             * (@c NULL if not applicable).
             */
            const char *context;
            /** The context mark (if @c problem_mark is not @c NULL). **/
            yaml_mark_t context_mark;
            /** The problem description. */
            const char *problem;
            /** The problem mark. */
            yaml_mark_t problem_mark;
        } loading;

        /** A problem while writing into the stream (@c YAML_WRITER_ERROR). */
        struct {
            /** The problem description. */
            const char *problem;
            /** The stream offset. */
            size_t offset;
        } writing;

        /** A problem while dumping into the stream (@c YAML_EMITTER_ERROR and
         * @c YAML_SERIALIZER_ERROR).
         */
        struct {
            /** The problem description. */
            const char *problem;
        } dumping;

        /** A problem while resolving an implicit tag (@c YAML_RESOLVER_ERROR). */
        struct {
            /** The problem description. */
            const char *problem;
        } resolving;

    } data;

} yaml_error_t;

/**
 * Create an error message.
 *
 * @param[in]   error   An error object.
 * @param[out]  buffer         model   A token to copy.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.  The function may
 * fail if the buffer is not large enough to contain the whole message.
 */

YAML_DECLARE(int)
yaml_error_message(yaml_error_t *error, char *buffer, size_t capacity);

/** @} */

/**
 * @defgroup basic Basic Types
 * @{
 */

/** The character type (UTF-8 octet). */
typedef unsigned char yaml_char_t;

/** The version directive data. */
typedef struct yaml_version_directive_s {
    /** The major version number. */
    int major;
    /** The minor version number. */
    int minor;
} yaml_version_directive_t;

/** The tag directive data. */
typedef struct yaml_tag_directive_s {
    /** The tag handle. */
    yaml_char_t *handle;
    /** The tag prefix. */
    yaml_char_t *prefix;
} yaml_tag_directive_t;

/** The stream encoding. */
typedef enum yaml_encoding_e {
    /** Let the parser choose the encoding. */
    YAML_ANY_ENCODING,
    /** The default UTF-8 encoding. */
    YAML_UTF8_ENCODING,
    /** The UTF-16-LE encoding with BOM. */
    YAML_UTF16LE_ENCODING,
    /** The UTF-16-BE encoding with BOM. */
    YAML_UTF16BE_ENCODING
} yaml_encoding_t;

/** Line break types. */
typedef enum yaml_break_e {
    /** Let the parser choose the break type. */
    YAML_ANY_BREAK,
    /** Use CR for line breaks (Mac style). */
    YAML_CR_BREAK,
    /** Use LN for line breaks (Unix style). */
    YAML_LN_BREAK,
    /** Use CR LN for line breaks (DOS style). */
    YAML_CRLN_BREAK
} yaml_break_t;

/** @} */

/**
 * @defgroup styles Node Styles
 * @{
 */

/** Scalar styles. */
typedef enum yaml_scalar_style_e {
    /** Let the emitter choose the style. */
    YAML_ANY_SCALAR_STYLE,

    /** The plain scalar style. */
    YAML_PLAIN_SCALAR_STYLE,

    /** The single-quoted scalar style. */
    YAML_SINGLE_QUOTED_SCALAR_STYLE,
    /** The double-quoted scalar style. */
    YAML_DOUBLE_QUOTED_SCALAR_STYLE,

    /** The literal scalar style. */
    YAML_LITERAL_SCALAR_STYLE,
    /** The folded scalar style. */
    YAML_FOLDED_SCALAR_STYLE
} yaml_scalar_style_t;

/** Sequence styles. */
typedef enum yaml_sequence_style_e {
    /** Let the emitter choose the style. */
    YAML_ANY_SEQUENCE_STYLE,

    /** The block sequence style. */
    YAML_BLOCK_SEQUENCE_STYLE,
    /** The flow sequence style. */
    YAML_FLOW_SEQUENCE_STYLE
} yaml_sequence_style_t;

/** Mapping styles. */
typedef enum yaml_mapping_style_e {
    /** Let the emitter choose the style. */
    YAML_ANY_MAPPING_STYLE,

    /** The block mapping style. */
    YAML_BLOCK_MAPPING_STYLE,
    /** The flow mapping style. */
    YAML_FLOW_MAPPING_STYLE
/*    YAML_FLOW_SET_MAPPING_STYLE   */
} yaml_mapping_style_t;

/** @} */

/**
 * @defgroup tokens Tokens
 * @{
 */

/** Token types. */
typedef enum yaml_token_type_e {
    /** An empty token. */
    YAML_NO_TOKEN,

    /** A STREAM-START token. */
    YAML_STREAM_START_TOKEN,
    /** A STREAM-END token. */
    YAML_STREAM_END_TOKEN,

    /** A VERSION-DIRECTIVE token. */
    YAML_VERSION_DIRECTIVE_TOKEN,
    /** A TAG-DIRECTIVE token. */
    YAML_TAG_DIRECTIVE_TOKEN,
    /** A DOCUMENT-START token. */
    YAML_DOCUMENT_START_TOKEN,
    /** A DOCUMENT-END token. */
    YAML_DOCUMENT_END_TOKEN,

    /** A BLOCK-SEQUENCE-START token. */
    YAML_BLOCK_SEQUENCE_START_TOKEN,
    /** A BLOCK-SEQUENCE-END token. */
    YAML_BLOCK_MAPPING_START_TOKEN,
    /** A BLOCK-END token. */
    YAML_BLOCK_END_TOKEN,

    /** A FLOW-SEQUENCE-START token. */
    YAML_FLOW_SEQUENCE_START_TOKEN,
    /** A FLOW-SEQUENCE-END token. */
    YAML_FLOW_SEQUENCE_END_TOKEN,
    /** A FLOW-MAPPING-START token. */
    YAML_FLOW_MAPPING_START_TOKEN,
    /** A FLOW-MAPPING-END token. */
    YAML_FLOW_MAPPING_END_TOKEN,

    /** A BLOCK-ENTRY token. */
    YAML_BLOCK_ENTRY_TOKEN,
    /** A FLOW-ENTRY token. */
    YAML_FLOW_ENTRY_TOKEN,
    /** A KEY token. */
    YAML_KEY_TOKEN,
    /** A VALUE token. */
    YAML_VALUE_TOKEN,

    /** An ALIAS token. */
    YAML_ALIAS_TOKEN,
    /** An ANCHOR token. */
    YAML_ANCHOR_TOKEN,
    /** A TAG token. */
    YAML_TAG_TOKEN,
    /** A SCALAR token. */
    YAML_SCALAR_TOKEN
} yaml_token_type_t;

/** The token structure. */
typedef struct yaml_token_s {

    /** The token type. */
    yaml_token_type_t type;

    /** The token data. */
    union {

        /** The stream start (for @c YAML_STREAM_START_TOKEN). */
        struct {
            /** The stream encoding. */
            yaml_encoding_t encoding;
        } stream_start;

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

    } data;

    /** The beginning of the token. */
    yaml_mark_t start_mark;
    /** The end of the token. */
    yaml_mark_t end_mark;

} yaml_token_t;

/**
 * Allocate a new empty token object.
 *
 * @returns a new token object or @c NULL on error.
 */

YAML_DECLARE(yaml_token_t *)
yaml_token_new(void);

/**
 * Delete and deallocate a token object.
 *
 * @param[in,out]   token   A token object.
 */

YAML_DECLARE(void)
yaml_token_delete(yaml_token_t *token);

/**
 * Duplicate a token object.
 *
 * @param[in,out]   token   An empty token object.
 * @param[in]       model   A token to copy.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_token_duplicate(yaml_token_t *token, const yaml_token_t *model);

/**
 * Free any memory allocated for a token object.
 *
 * @param[in,out]   token   A token object.
 */

YAML_DECLARE(void)
yaml_token_destroy(yaml_token_t *token);

/** @} */

/**
 * @defgroup events Events
 * @{
 */

/** Event types. */
typedef enum yaml_event_type_e {
    /** An empty event. */
    YAML_NO_EVENT,

    /** A STREAM-START event. */
    YAML_STREAM_START_EVENT,
    /** A STREAM-END event. */
    YAML_STREAM_END_EVENT,

    /** A DOCUMENT-START event. */
    YAML_DOCUMENT_START_EVENT,
    /** A DOCUMENT-END event. */
    YAML_DOCUMENT_END_EVENT,

    /** An ALIAS event. */
    YAML_ALIAS_EVENT,
    /** A SCALAR event. */
    YAML_SCALAR_EVENT,

    /** A SEQUENCE-START event. */
    YAML_SEQUENCE_START_EVENT,
    /** A SEQUENCE-END event. */
    YAML_SEQUENCE_END_EVENT,

    /** A MAPPING-START event. */
    YAML_MAPPING_START_EVENT,
    /** A MAPPING-END event. */
    YAML_MAPPING_END_EVENT
} yaml_event_type_t;

/** The event structure. */
typedef struct yaml_event_s {

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
                /** The beginning of the list. */
                yaml_tag_directive_t *list;
                /** The length of the list. */
                size_t length;
                /** The capacity of the list. */
                size_t capacity;
            } tag_directives;

            /** Is the document indicator implicit? */
            int is_implicit;
        } document_start;

        /** The document end parameters (for @c YAML_DOCUMENT_END_EVENT). */
        struct {
            /** Is the document end indicator implicit? */
            int is_implicit;
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
            int is_plain_implicit;
            /** Is the tag optional for any non-plain style? */
            int is_quoted_implicit;
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
            int is_implicit;
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
            int is_implicit;
            /** The mapping style. */
            yaml_mapping_style_t style;
        } mapping_start;

    } data;

    /** The beginning of the event. */
    yaml_mark_t start_mark;
    /** The end of the event. */
    yaml_mark_t end_mark;

} yaml_event_t;

/**
 * Allocate a new empty event object.
 *
 * @returns a new event object or @c NULL on error.
 */

YAML_DECLARE(yaml_event_t *)
yaml_event_new(void);

/**
 * Delete and deallocate an event object.
 *
 * @param[in,out]   event   An event object.
 */

YAML_DECLARE(void)
yaml_event_delete(yaml_event_t *event);

/**
 * Duplicate a event object.
 *
 * @param[in,out]   event   An empty event object.
 * @param[in]       model   An event to copy.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_duplicate(yaml_event_t *event, const yaml_event_t *model);

/**
 * Create a STREAM-START event.
 *
 * This function never fails.
 *
 * @param[out]      event       An empty event object.
 * @param[in]       encoding    The stream encoding.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_stream_start(yaml_event_t *event,
        yaml_encoding_t encoding);

/**
 * Create a STREAM-END event.
 *
 * This function never fails.
 *
 * @param[out]      event       An empty event object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_stream_end(yaml_event_t *event);

/**
 * Create the DOCUMENT-START event.
 *
 * @param[out]      event                   An empty event object.
 * @param[in]       version_directive       The %YAML directive value or
 *                                          @c NULL.
 * @param[in]       tag_directives_list     The beginning of the %TAG
 *                                          directives list or @c NULL.  The
 *                                          list ends with @c (NULL,NULL).
 * @param[in]       is_implicit             Set if the document start
 *                                          indicator is optional.  This
 *                                          parameter is stylistic and may be
 *                                          ignored by the parser.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_document_start(yaml_event_t *event,
        const yaml_version_directive_t *version_directive,
        const yaml_tag_directive_t *tag_directives,
        int is_implicit);

/**
 * Create the DOCUMENT-END event.
 *
 * @param[out]      event       An empty event object.
 * @param[in]       is_implicit Set if the document end indicator is optional.
 *                              This parameter is stylistic and may be ignored
 *                              by the parser.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_document_end(yaml_event_t *event, int is_implicit);

/**
 * Create an ALIAS event.
 *
 * @param[out]      event       An empty event object.
 * @param[in]       anchor      The anchor value.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_alias(yaml_event_t *event, const yaml_char_t *anchor);

/**
 * Create a SCALAR event.
 *
 * @param[out]      event                   An empty event object.
 * @param[in]       anchor                  The scalar anchor or @c NULL.
 * @param[in]       tag                     The scalar tag or @c NULL.  If
 *                                          latter, one of the
 *                                          @a is_plain_implicit and
 *                                          @a is_quoted_implicit flags must
 *                                          be set.
 * @param[in]       value                   The scalar value.
 * @param[in]       length                  The length of the scalar value.
 * @param[in]       is_plain_implicit       Set if the tag may be omitted for
 *                                          the plain style.
 * @param[in]       is_quoted_implicit      Set if the tag may be omitted for
 *                                          any non-plain style.
 * @param[in]       style                   The scalar style.  This attribute
 *                                          may be ignored by the emitter.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_scalar(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        const yaml_char_t *value, size_t length,
        int is_plain_implicit, int is_quoted_implicit,
        yaml_scalar_style_t style);

/**
 * Create a SEQUENCE-START event.
 *
 * @param[out]      event       An empty event object.
 * @param[in]       anchor      The sequence anchor or @c NULL.
 * @param[in]       tag         The sequence tag or @c NULL.  If latter, the
 *                              @a is_implicit flag must be set.
 * @param[in]       is_implicit Set if the tag may be omitted.
 * @param[in]       style       The sequence style.  This attribute may be
 *                              ignored by the parser.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_sequence_start(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        int is_implicit, yaml_sequence_style_t style);

/**
 * Create a SEQUENCE-END event.
 *
 * @param[out]      event       An empty event object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_sequence_end(yaml_event_t *event);

/**
 * Create a MAPPING-START event.
 *
 * @param[out]      event       An empty event object.
 * @param[in]       anchor      The mapping anchor or @c NULL.
 * @param[in]       tag         The mapping tag or @c NULL.  If latter, the
 *                              @a is_implicit flag must be set.
 * @param[in]       is_implicit Set if the tag may be omitted.
 * @param[in]       style       The mapping style.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_mapping_start(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        int is_implicit, yaml_mapping_style_t style);

/**
 * Create a MAPPING-END event.
 *
 * @param[out]      event       An empty event object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_event_create_mapping_end(yaml_event_t *event);

/**
 * Free any memory allocated for an event object.
 *
 * @param[in,out]   event   An event object.
 */

YAML_DECLARE(void)
yaml_event_destroy(yaml_event_t *event);

/** @} */

/**
 * @defgroup nodes Nodes
 * @{
 */

/** The tag @c !!null with the only possible value: @c null. */
#define YAML_NULL_TAG       ((const yaml_char_t *) "tag:yaml.org,2002:null")
/** The tag @c !!bool with the values: @c true and @c falce. */
#define YAML_BOOL_TAG       ((const yaml_char_t *) "tag:yaml.org,2002:bool")
/** The tag @c !!str for string values. */
#define YAML_STR_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:str")
/** The tag @c !!int for integer values. */
#define YAML_INT_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:int")
/** The tag @c !!float for float values. */
#define YAML_FLOAT_TAG      ((const yaml_char_t *) "tag:yaml.org,2002:float")

/** The tag @c !!seq is used to denote sequences. */
#define YAML_SEQ_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:seq")
/** The tag @c !!map is used to denote mapping. */
#define YAML_MAP_TAG        ((const yaml_char_t *) "tag:yaml.org,2002:map")

/** The default scalar tag is @c !!str. */
#define YAML_DEFAULT_SCALAR_TAG     YAML_STR_TAG
/** The default sequence tag is @c !!seq. */
#define YAML_DEFAULT_SEQUENCE_TAG   YAML_SEQ_TAG
/** The default mapping tag is @c !!map. */
#define YAML_DEFAULT_MAPPING_TAG    YAML_MAP_TAG

/** Node types. */
typedef enum yaml_node_type_e {
    /** An empty node. */
    YAML_NO_NODE,

    /** A scalar node. */
    YAML_SCALAR_NODE,
    /** A sequence node. */
    YAML_SEQUENCE_NODE,
    /** A mapping node. */
    YAML_MAPPING_NODE
} yaml_node_type_t;

/** Arc types. */
typedef enum yaml_arc_type_e {
    /** An empty arc. */
    YAML_NO_ARC,

    /** An item of a sequence. */
    YAML_SEQUENCE_ITEM_ARC,
    /** A key of a mapping. */
    YAML_MAPPING_KEY_ARC,
    /** A value of a mapping. */
    YAML_MAPPING_VALUE_ARC
} yaml_arc_type_t;

/** The forward definition of a document node structure. */
typedef struct yaml_node_s yaml_node_t;

/** An element of a sequence node. */
typedef int yaml_node_item_t;

/** An element of a mapping node. */
typedef struct yaml_node_pair_s {
    /** The key of the element. */
    int key;
    /** The value of the element. */
    int value;
} yaml_node_pair_t;

/** An element of a path in a YAML document. */
typedef struct yaml_arc_s {
    /** The arc type. */
    yaml_arc_type_t type;
    /** The collection node. */
    int node;
    /** A pointer in the collection node. */
    int item;
} yaml_arc_t;

/** The node structure. */
struct yaml_node_s {

    /** The node type. */
    yaml_node_type_t type;

    /** The node anchor. */
    yaml_char_t *anchor;
    /** The node tag. */
    yaml_char_t *tag;

    /** The node data. */
    union {
        
        /** The scalar parameters (for @c YAML_SCALAR_NODE). */
        struct {
            /** The scalar value. */
            yaml_char_t *value;
            /** The length of the scalar value. */
            size_t length;
            /** The scalar style. */
            yaml_scalar_style_t style;
        } scalar;

        /** The sequence parameters (for @c YAML_SEQUENCE_NODE). */
        struct {
            /** The list of sequence items. */
            struct {
                /** The beginning of the list. */
                yaml_node_item_t *list;
                /** The length of the list. */
                size_t length;
                /** The capacity of the list. */
                size_t capacity;
            } items;
            /** The sequence style. */
            yaml_sequence_style_t style;
        } sequence;

        /** The mapping parameters (for @c YAML_MAPPING_NODE). */
        struct {
            /** The list of mapping pairs (key, value). */
            struct {
                /** The beginning of the list. */
                yaml_node_pair_t *list;
                /** The length of the list. */
                size_t length;
                /** The capacity of the list. */
                size_t capacity;
            } pairs;
            /** The mapping style. */
            yaml_mapping_style_t style;
        } mapping;

    } data;

    /** The beginning of the node. */
    yaml_mark_t start_mark;
    /** The end of the node. */
    yaml_mark_t end_mark;

};

/** The incomplete node structure. */
typedef struct yaml_incomplete_node_s {

    /** The node type. */
    yaml_node_type_t type;

    /** The path to the new node. */
    struct {
        /** The beginning of the list. */
        yaml_arc_t *list;
        /** The length of the list. */
        size_t length;
        /** The capacity of the list. */
        size_t capacity;
    } path;

    /** The node data. */
    union {

        /** The scalar parameters (for @c YAML_SCALAR_NODE). */
        struct {
            /** The scalar value. */
            yaml_char_t *value;
            /** The length of the scalar value. */
            size_t length;
            /** Set if the scalar is plain. */
            int is_plain;
        } scalar;

    } data;

    /** The position of the node. */
    yaml_mark_t mark;

} yaml_incomplete_node_t;

/** The document structure. */
typedef struct yaml_document_s {

    /** The document nodes. */
    struct {
        /** The beginning of the list. */
        yaml_node_t *list;
        /** The length of the list. */
        size_t length;
        /** The capacity of the list. */
        size_t capacity;
    } nodes;

    /** The version directive. */
    yaml_version_directive_t *version_directive;

    /** The list of tag directives. */
    struct {
        /** The beginning of the tag directive list. */
        yaml_tag_directive_t *list;
        /** The length of the tag directive list. */
        size_t length;
        /** The capacity of the tag directive list. */
        size_t capacity;
    } tag_directives;

    /** Is the document start indicator implicit? */
    int is_start_implicit;
    /** Is the document end indicator implicit? */
    int is_end_implicit;

    /** The beginning of the document. */
    yaml_mark_t start_mark;
    /** The end of the document. */
    yaml_mark_t end_mark;

} yaml_document_t;

#if 0

/**
 * Allocate a new empty document object.
 *
 * @returns a new document object or @c NULL on error.
 */

YAML_DECLARE(yaml_document_t *)
yaml_document_new(void);

/**
 * Delete and deallocatge a document object.
 *
 * @param[in,out]   document    A document object.
 */

YAML_DECLARE(void)
yaml_document_delete(yaml_document_t *document);

/**
 * Duplicate a document object.
 *
 * @param[in,out]   document   An empty document object.
 * @param[in]       model       A document to copy.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_document_duplicate(yaml_document_t *document, yaml_document_t *model);

/**
 * Create a YAML document.
 *
 * @param[out]      document                An empty document object.
 * @param[in]       version_directive       The %YAML directive value or
 *                                          @c NULL.
 * @param[in]       tag_directives          The list of the %TAG directives or
 *                                          @c NULL.  The list must end with
 *                                          the pair @c (NULL,NULL).
 * @param[in]       is_start_implicit       If the document start indicator is
 *                                          implicit.
 * @param[in]       is_end_implicit         If the document end indicator is
 *                                          implicit.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_document_create(yaml_document_t *document,
        yaml_version_directive_t *version_directive,
        yaml_tag_directive_t *tag_directives,
        int is_start_implicit, int is_end_implicit);

/**
 * Delete a YAML document and all its nodes.
 *
 * @param[in,out]   document        A document object.
 */

YAML_DECLARE(void)
yaml_document_destroy(yaml_document_t *document);

/**
 * Get a node of a YAML document.
 *
 * The pointer returned by this function is valid until any of the functions
 * modifying the documents is called.
 *
 * @param[in]       document        A document object.
 * @param[in]       index           The node id.
 *
 * @returns the node objct or @c NULL if @c node_id is out of range.
 */

YAML_DECLARE(yaml_node_t *)
yaml_document_get_node(yaml_document_t *document, int index);

/**
 * Get the root of a YAML document node.
 *
 * The root object is the first object added to the document.
 *
 * The pointer returned by this function is valid until any of the functions
 * modifying the documents is called.
 *
 * An empty document produced by the parser signifies the end of a YAML
 * stream.
 *
 * @param[in]       document        A document object.
 *
 * @returns the node object or @c NULL if the document is empty.
 */

YAML_DECLARE(yaml_node_t *)
yaml_document_get_root_node(yaml_document_t *document);

/**
 * Create a SCALAR node and attach it to the document.
 *
 * The @a style argument may be ignored by the emitter.
 *
 * @param[in,out]   document        A document object.
 * @param[in]       anchor          A preferred anchor for the node or @c NULL.
 * @param[in]       tag             The scalar tag.
 * @param[in]       value           The scalar value.
 * @param[in]       length          The length of the scalar value.
 * @param[in]       style           The scalar style.
 *
 * @returns the node id or @c 0 on error.
 */

YAML_DECLARE(int)
yaml_document_add_scalar(yaml_document_t *document,
        yaml_char_t *anchor, yaml_char_t *tag, yaml_char_t *value,
        size_t length, yaml_scalar_style_t style);

/**
 * Create a SEQUENCE node and attach it to the document.
 *
 * The @a style argument may be ignored by the emitter.
 *
 * @param[in,out]   document    A document object.
 * @param[in]       anchor      A preferred anchor for the node or @c NULL.
 * @param[in]       tag         The sequence tag.
 * @param[in]       style       The sequence style.
 *
 * @returns the node id or @c 0 on error.
 */

YAML_DECLARE(int)
yaml_document_add_sequence(yaml_document_t *document,
        yaml_char_t *anchor, yaml_char_t *tag, yaml_sequence_style_t style);

/**
 * Create a MAPPING node and attach it to the document.
 *
 * The @a style argument may be ignored by the emitter.
 *
 * @param[in,out]   document    A document object.
 * @param[in]       anchor      A preferred anchor for the node or @c NULL.
 * @param[in]       tag         The sequence tag.
 * @param[in]       style       The sequence style.
 *
 * @returns the node id or @c 0 on error.
 */

YAML_DECLARE(int)
yaml_document_add_mapping(yaml_document_t *document,
        yaml_char_t *anchor, yaml_char_t *tag, yaml_mapping_style_t style);

/**
 * Add an item to a SEQUENCE node.
 *
 * @param[in,out]   document    A document object.
 * @param[in]       sequence    The sequence node id.
 * @param[in]       item        The item node id.
*
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_document_append_sequence_item(yaml_document_t *document,
        int sequence, int item);

/**
 * Add a pair of a key and a value to a MAPPING node.
 *
 * @param[in,out]   document    A document object.
 * @param[in]       mapping     The mapping node id.
 * @param[in]       key         The key node id.
 * @param[in]       value       The value node id.
*
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_document_append_mapping_pair(yaml_document_t *document,
        int mapping, int key, int value);

#endif

/**
 * @defgroup callbacks Callback Definitions
 * @{
 */

/**
 * The prototype of a read handler.
 *
 * The read handler is called when the parser needs to read more bytes from the
 * source.  The handler should read no more than @a size bytes and write them
 * to the @a buffer.  The number of the read bytes should be returned using the
 * @a size_read variable.  If the handler reaches the stream end, @a size_read
 * must be set to @c 0.
 *
 * @param[in,out]   data        A pointer to an application data specified by
 *                              @c yaml_parser_set_reader().
 * @param[out]      buffer      The buffer to write the data from the source.
 * @param[in]       capacity    The maximum number of bytes the buffer can hold.
 * @param[out]      length      The actual number of bytes read from the source.
 *
 * @returns On success, the handler should return @c 1.  If the handler failed,
 * the returned value should be @c 0.  On EOF, the handler should set the
 * @a size_read to @c 0 and return @c 1.
 */

typedef int yaml_reader_t(void *data, unsigned char *buffer, size_t capacity,
        size_t *length);

/**
 * The prototype of a write handler.
 *
 * The write handler is called when the emitter needs to flush the accumulated
 * characters to the output.  The handler should write @a size bytes of the
 * @a buffer to the output.
 *
 * @param[in,out]   data        A pointer to an application data specified by
 *                              @c yaml_emitter_set_writer().
 * @param[in]       buffer      The buffer with bytes to be written.
 * @param[in]       length      The number of bytes to be written.
 *
 * @returns On success, the handler should return @c 1.  If the handler failed,
 * it should return @c 0.
 */

typedef int yaml_writer_t(void *data, const unsigned char *buffer,
        size_t length);

/**
 * The prototype of a tag resolver.
 *
 * The resolve handler is called when the parser encounters a new node without
 * an explicit tag.  The handler should determine the correct tag of the node
 * basing on the node kind, the path to the node from the document root and,
 * in the case of the scalar node, the node value.  The handler is also called
 * by the emitter to determine whether the node tag could be omitted.
 *
 * @param[in,out]   data        A pointer to an application data specified by
 *                              @c yaml_parser_set_writter() or
 *                              @c yaml_emitter_set_writer().
 * @param[in]       node        Information about the new node.
 * @param[out]      tag         The guessed node tag.
 *
 * @returns On success, the handler should return @c 1.  If the handler failed,
 * it should return @c 0.
 */

typedef int yaml_resolver_t(void *data, yaml_incomplete_node_t *node,
        yaml_char_t **tag);

/** @} */

/**
 * @defgroup parser Parser Definitions
 * @{
 */

/** The parser object. */
typedef struct yaml_parser_s yaml_parser_t;

/**
 * Create a new parser object.
 *
 * This function creates a new parser object.  An application is responsible
 * for destroying the object using the @c yaml_parser_delete() function.
 *
 * @returns a new parser object or @c NULL on error.
 */

YAML_DECLARE(yaml_parser_t *)
yaml_parser_new(void);

/**
 * Destroy a parser.
 *
 * @param[in,out]   parser  A parser object.
 */

YAML_DECLARE(void)
yaml_parser_delete(yaml_parser_t *parser);

/**
 * Get a parser error.
 *
 * @param[in]   parser  A parser object.
 * @param[out]  error   An error object.
 */

YAML_DECLARE(void)
yaml_parser_get_error(yaml_parser_t *parser, yaml_error_t *error);

/**
 * Set a string input.
 *
 * Note that the @a input pointer must be valid while the @a parser object
 * exists.  The application is responsible for destroing @a input after
 * destroying the @a parser.
 *
 * @param[in,out]   parser  A parser object.
 * @param[in]       buffer  A source data.
 * @param[in]       length  The length of the source data in bytes.
 */

YAML_DECLARE(void)
yaml_parser_set_string_reader(yaml_parser_t *parser,
        const unsigned char *buffer, size_t length);

/**
 * Set a file input.
 *
 * @a file should be a file object open for reading.  The application is
 * responsible for closing the @a file.
 *
 * @param[in,out]   parser  A parser object.
 * @param[in]       file    An open file.
 */

YAML_DECLARE(void)
yaml_parser_set_file_reader(yaml_parser_t *parser, FILE *file);

/**
 * Set a generic input handler.
 *
 * @param[in,out]   parser  A parser object.
 * @param[in]       reader  A read handler.
 * @param[in]       data    Any application data for passing to the read
 *                          handler.
 */

YAML_DECLARE(void)
yaml_parser_set_reader(yaml_parser_t *parser,
        yaml_reader_t *reader, void *data);

#if 0

/**
 * Set the standard resolve handler.
 *
 * The standard resolver recognize the following scalar kinds: !!null, !!bool,
 * !!str, !!int and !!float.
 *
 * @param[in,out]   parser  A parser object.
 */

YAML_DECLARE(void)
yaml_parser_set_standard_resolver(yaml_parser_t *parser);

/**
 * Set the resolve handler.
 *
 * The standard resolver recognize the following scalar kinds: !!null, !!bool,
 * !!str, !!int and !!float.
 *
 * @param[in,out]   parser      A parser object.
 * @param[in]       resolver    A resolve handler.
 * @param[in]       data        Any application data for passing to the resolve
 *                              handler.
 */

YAML_DECLARE(void)
yaml_parser_set_resolver(yaml_parser_t *parser,
        yaml_resolver_t *resolver, void *data);

#endif

/**
 * Set the source encoding.
 *
 * @param[in,out]   parser      A parser object.
 * @param[in]       encoding    The source encoding.
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
 * produced token object using the @c yaml_token_delete() function.
 *
 * An application must not alternate the calls of @c yaml_parser_scan() with
 * the calls of @c yaml_parser_parse() or @c yaml_parser_load(). Doing this
 * will break the parser.
 *
 * @param[in,out]   parser      A parser object.
 * @param[out]      token       An empty token object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_parser_parse_token(yaml_parser_t *parser, yaml_token_t *token);

/**
 * Parse the input stream and produce the next parsing event.
 *
 * Call the function subsequently to produce a sequence of events corresponding
 * to the input stream.  The initial event has the type
 * @c YAML_STREAM_START_EVENT while the ending event has the type
 * @c YAML_STREAM_END_EVENT.
 *
 * An application is responsible for freeing any buffers associated with the
 * produced event object using the @c yaml_event_delete() function.
 *
 * An application must not alternate the calls of @c yaml_parser_parse() with
 * the calls of @c yaml_parser_scan() or @c yaml_parser_load(). Doing this will
 * break the parser.
 *
 * @param[in,out]   parser      A parser object.
 * @param[out]      event       An empty event object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_parser_parse_event(yaml_parser_t *parser, yaml_event_t *event);

#if 0

/**
 * Parse the input stream and produce the next YAML document.
 *
 * Call this function subsequently to produce a sequence of documents
 * constituting the input stream.
 *
 * If the produced document has no root node, it means that the document
 * end has been reached.
 *
 * An application is responsible for freeing any data associated with the
 * produced document object using the @c yaml_document_delete() function.
 *
 * An application must not alternate the calls of @c yaml_parser_load() with
 * the calls of @c yaml_parser_scan() or @c yaml_parser_parse(). Doing this
 * will break the parser.
 *
 * @param[in,out]   parser      A parser object.
 * @param[out]      document    An empty document object.
 *
 * @return @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_parser_parse_document(yaml_parser_t *parser, yaml_document_t *document);

#endif

/** @} */

/**
 * @defgroup emitter Emitter Definitions
 * @{
 */

/** The emitter object. */
typedef struct yaml_emitter_s yaml_emitter_t;

/**
 * Create a new emitter object.
 *
 * This function creates a new emitter object.  An application is responsible
 * for destroying the object using the @c yaml_emitter_delete() function.
 *
 * @returns a new emitter object or @c NULL on error.
 */

YAML_DECLARE(yaml_emitter_t *)
yaml_emitter_new(void);

/**
 * Destroy an emitter.
 *
 * @param[in,out]   emitter     An emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_delete(yaml_emitter_t *emitter);

/**
 * Get an emitter error.
 *
 * @param[in]   emitter An emitter object.
 * @param[out]  error   An error object.
 */

YAML_DECLARE(void)
yaml_emitter_get_error(yaml_emitter_t *emitter, yaml_error_t *error);

/**
 * Set a string output.
 *
 * The emitter will write the output characters to the @a output buffer of the
 * size @a size.  The emitter will set @a size_written to the number of written
 * bytes.  If the buffer is smaller than required, the emitter produces the
 * @c YAML_WRITE_ERROR error.
 *
 * @param[in,out]   emitter         An emitter object.
 * @param[in]       buffer          An output buffer.
 * @param[in]       length          The pointer to save the number of written
 *                                  bytes.
 * @param[in]       capacity        The buffer size.
 */

YAML_DECLARE(void)
yaml_emitter_set_string_writer(yaml_emitter_t *emitter,
        unsigned char *buffer, size_t *length, size_t capacity);

/**
 * Set a file output.
 *
 * @a file should be a file object open for writing.  The application is
 * responsible for closing the @a file.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       file        An open file.
 */

YAML_DECLARE(void)
yaml_emitter_set_file_writer(yaml_emitter_t *emitter, FILE *file);

/**
 * Set a generic output handler.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       writer      A write handler.
 * @param[in]       data        Any application data for passing to the write
 *                              handler.
 */

YAML_DECLARE(void)
yaml_emitter_set_writer(yaml_emitter_t *emitter,
        yaml_writer_t *writer, void *data);

#if 0

/**
 * Set the standard resolve handler.
 *
 * The standard resolver recognize the following scalar kinds: !!null, !!bool,
 * !!str, !!int and !!float.
 *
 * @param[in,out]   emitter An emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_set_standard_resolver(yaml_emitter_t *emitter);

/**
 * Set the resolve handler.
 *
 * The standard resolver recognize the following scalar kinds: !!null, !!bool,
 * !!str, !!int and !!float.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       resolver    A resolve handler.
 * @param[in]       data        Any application data for passing to the resolve
 *                              handler.
 */

YAML_DECLARE(void)
yaml_emitter_set_resolver(yaml_emitter_t *emitter,
        yaml_resolver_t *resolver, void *data);

#endif

/**
 * Set the output encoding.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       encoding    The output encoding.
 */

YAML_DECLARE(void)
yaml_emitter_set_encoding(yaml_emitter_t *emitter, yaml_encoding_t encoding);

/**
 * Set if the output should be in the "canonical" format as in the YAML
 * specification.
 *
 * @param[in,out]   emitter         An emitter object.
 * @param[in]       is_canonical    If the output is canonical.
 */

YAML_DECLARE(void)
yaml_emitter_set_canonical(yaml_emitter_t *emitter, int is_canonical);

/**
 * Set the intendation increment.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       indent      The indentation increment (1 < . < 10).
 */

YAML_DECLARE(void)
yaml_emitter_set_indent(yaml_emitter_t *emitter, int indent);

/**
 * Set the preferred line width. @c -1 means unlimited.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       width       The preferred line width.
 */

YAML_DECLARE(void)
yaml_emitter_set_width(yaml_emitter_t *emitter, int width);

/**
 * Set if unescaped non-ASCII characters are allowed.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       is_unicode  If unescaped Unicode characters are allowed.
 */

YAML_DECLARE(void)
yaml_emitter_set_unicode(yaml_emitter_t *emitter, int is_unicode);

/**
 * Set the preferred line break.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in]       line_break  The preferred line break.
 */

YAML_DECLARE(void)
yaml_emitter_set_break(yaml_emitter_t *emitter, yaml_break_t line_break);

/**
 * Emit an event.
 *
 * The event object may be generated using the yaml_parser_parse() function.
 * The emitter takes the responsibility for the event object and destroys its
 * content after it is emitted. The event object is destroyed even if the
 * function fails.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in,out]   event       An event object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_event(yaml_emitter_t *emitter, yaml_event_t *event);

#if 0

/**
 * Start a YAML stream.
 *
 * This function should be used before the first @c yaml_emitter_dump() is
 * called.
 *
 * @param[in,out]   emitter     An emitter object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_open(yaml_emitter_t *emitter);

/**
 * Finish a YAML stream.
 *
 * This function should be used after the last @c yaml_emitter_dump() is
 * called.
 *
 * @param[in,out]   emitter     An emitter object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_close(yaml_emitter_t *emitter);

/**
 * Emit a YAML document.
 *
 * The document object may be generated using the @c yaml_parser_load()
 * function or the @c yaml_document_initialize() function.  The emitter takes
 * the responsibility for the document object and destoys its content after
 * it is emitted. The document object is destroyed even if the function fails.
 *
 * @param[in,out]   emitter     An emitter object.
 * @param[in,out]   document    A document object.
 *
 * @returns @c 1 if the function succeeded, @c 0 on error.
 */

YAML_DECLARE(int)
yaml_emitter_emit_document(yaml_emitter_t *emitter, yaml_document_t *document);

#endif

/**
 * Flush the accumulated characters to the output stream.
 *
 * @param[in,out]   emitter     An emitter object.
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

