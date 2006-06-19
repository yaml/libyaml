
/*
 * Introduction
 * ************
 *
 * The following notes assume that you are familiar with the YAML specification
 * (http://yaml.org/spec/cvs/current.html).  We mostly follow it, although in
 * some cases we are less restrictive that it requires.
 *
 * The process of transforming a YAML stream into a sequence of events is
 * divided on two steps: Scanning and Parsing.
 *
 * The Scanner transforms the input stream into a sequence of tokens, while the
 * parser transform the sequence of tokens produced by the Scanner into a
 * sequence of parsing events.
 *
 * The Scanner is rather clever and complicated. The Parser, on the contrary,
 * is a straightforward implementation of a recursive-descendant parser (or,
 * LL(1) parser, as it is usually called).
 *
 * Actually there are two issues of Scanning that might be called "clever", the
 * rest is quite straightforward.  The issues are "block collection start" and
 * "simple keys".  Both issues are explained below in details.
 *
 * Here the Scanning step is explained and implemented.  We start with the list
 * of all the tokens produced by the Scanner together with short descriptions.
 *
 * Now, tokens:
 *
 *      STREAM-START(encoding)          # The stream start.
 *      STREAM-END                      # The stream end.
 *      VERSION-DIRECTIVE(major,minor)  # The '%YAML' directive.
 *      TAG-DIRECTIVE(handle,prefix)    # The '%TAG' directive.
 *      DOCUMENT-START                  # '---'
 *      DOCUMENT-END                    # '...'
 *      BLOCK-SEQUENCE-START            # Indentation increase denoting a block
 *      BLOCK-MAPPING-START             # sequence or a block mapping.
 *      BLOCK-END                       # Indentation decrease.
 *      FLOW-SEQUENCE-START             # '['
 *      FLOW-SEQUENCE-END               # ']'
 *      BLOCK-SEQUENCE-START            # '{'
 *      BLOCK-SEQUENCE-END              # '}'
 *      BLOCK-ENTRY                     # '-'
 *      FLOW-ENTRY                      # ','
 *      KEY                             # '?' or nothing (simple keys).
 *      VALUE                           # ':'
 *      ALIAS(anchor)                   # '*anchor'
 *      ANCHOR(anchor)                  # '&anchor'
 *      TAG(handle,suffix)              # '!handle!suffix'
 *      SCALAR(value,style)             # A scalar.
 *
 * The following two tokens are "virtual" tokens denoting the beginning and the
 * end of the stream:
 *
 *      STREAM-START(encoding)
 *      STREAM-END
 *
 * We pass the information about the input stream encoding with the
 * STREAM-START token.
 *
 * The next two tokens are responsible for tags:
 *
 *      VERSION-DIRECTIVE(major,minor)
 *      TAG-DIRECTIVE(handle,prefix)
 *
 * Example:
 *
 *      %YAML   1.1
 *      %TAG    !   !foo
 *      %TAG    !yaml!  tag:yaml.org,2002:
 *      ---
 *
 * The correspoding sequence of tokens:
 *
 *      STREAM-START(utf-8)
 *      VERSION-DIRECTIVE(1,1)
 *      TAG-DIRECTIVE("!","!foo")
 *      TAG-DIRECTIVE("!yaml","tag:yaml.org,2002:")
 *      DOCUMENT-START
 *      STREAM-END
 *
 * Note that the VERSION-DIRECTIVE and TAG-DIRECTIVE tokens occupy a whole
 * line.
 *
 * The document start and end indicators are represented by:
 *
 *      DOCUMENT-START
 *      DOCUMENT-END
 *
 * Note that if a YAML stream contains an implicit document (without '---'
 * and '...' indicators), no DOCUMENT-START and DOCUMENT-END tokens will be
 * produced.
 *
 * In the following examples, we present whole documents together with the
 * produced tokens.
 *
 *      1. An implicit document:
 *
 *          'a scalar'
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          SCALAR("a scalar",single-quoted)
 *          STREAM-END
 *
 *      2. An explicit document:
 *
 *          ---
 *          'a scalar'
 *          ...
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          DOCUMENT-START
 *          SCALAR("a scalar",single-quoted)
 *          DOCUMENT-END
 *          STREAM-END
 *
 *      3. Several documents in a stream:
 *
 *          'a scalar'
 *          ---
 *          'another scalar'
 *          ---
 *          'yet another scalar'
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          SCALAR("a scalar",single-quoted)
 *          DOCUMENT-START
 *          SCALAR("another scalar",single-quoted)
 *          DOCUMENT-START
 *          SCALAR("yet another scalar",single-quoted)
 *          STREAM-END
 *
 * We have already introduced the SCALAR token above.  The following tokens are
 * used to describe aliases, anchors, tag, and scalars:
 *
 *      ALIAS(anchor)
 *      ANCHOR(anchor)
 *      TAG(handle,suffix)
 *      SCALAR(value,style)
 *
 * The following series of examples illustrate the usage of these tokens:
 *
 *      1. A recursive sequence:
 *
 *          &A [ *A ]
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          ANCHOR("A")
 *          FLOW-SEQUENCE-START
 *          ALIAS("A")
 *          FLOW-SEQUENCE-END
 *          STREAM-END
 *
 *      2. A tagged scalar:
 *
 *          !!float "3.14"  # A good approximation.
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          TAG("!!","float")
 *          SCALAR("3.14",double-quoted)
 *          STREAM-END
 *
 *      3. Various scalar styles:
 *
 *          --- # Implicit empty plain scalars do not produce tokens.
 *          --- a plain scalar
 *          --- 'a single-quoted scalar'
 *          --- "a double-quoted scalar"
 *          --- |-
 *            a literal scalar
 *          --- >-
 *            a folded
 *            scalar
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          DOCUMENT-START
 *          DOCUMENT-START
 *          SCALAR("a plain scalar",plain)
 *          DOCUMENT-START
 *          SCALAR("a single-quoted scalar",single-quoted)
 *          DOCUMENT-START
 *          SCALAR("a double-quoted scalar",double-quoted)
 *          DOCUMENT-START
 *          SCALAR("a literal scalar",literal)
 *          DOCUMENT-START
 *          SCALAR("a folded scalar",folded)
 *          STREAM-END
 *
 * Now it's time to review collection-related tokens. We will start with
 * flow collections:
 *
 *      FLOW-SEQUENCE-START
 *      FLOW-SEQUENCE-END
 *      FLOW-MAPPING-START
 *      FLOW-MAPPING-END
 *      FLOW-ENTRY
 *      KEY
 *      VALUE
 *
 * The tokens FLOW-SEQUENCE-START, FLOW-SEQUENCE-END, FLOW-MAPPING-START, and
 * FLOW-MAPPING-END represent the indicators '[', ']', '{', and '}'
 * correspondingly.  FLOW-ENTRY represent the ',' indicator.  Finally the
 * indicators '?' and ':', which are used for denoting mapping keys and values,
 * are represented by the KEY and VALUE tokens.
 *
 * The following examples show flow collections:
 *
 *      1. A flow sequence:
 *
 *          [item 1, item 2, item 3]
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          FLOW-SEQUENCE-START
 *          SCALAR("item 1",plain)
 *          FLOW-ENTRY
 *          SCALAR("item 2",plain)
 *          FLOW-ENTRY
 *          SCALAR("item 3",plain)
 *          FLOW-SEQUENCE-END
 *          STREAM-END
 *
 *      2. A flow mapping:
 *
 *          {
 *              a simple key: a value,  # Note that the KEY token is produced.
 *              ? a complex key: another value,
 *          }
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          FLOW-MAPPING-START
 *          KEY
 *          SCALAR("a simple key",plain)
 *          VALUE
 *          SCALAR("a value",plain)
 *          FLOW-ENTRY
 *          KEY
 *          SCALAR("a complex key",plain)
 *          VALUE
 *          SCALAR("another value",plain)
 *          FLOW-ENTRY
 *          FLOW-MAPPING-END
 *          STREAM-END
 *
 * A simple key is a key which is not denoted by the '?' indicator.  Note that
 * the Scanner still produce the KEY token whenever it encounters a simple key.
 *
 * For scanning block collections, the following tokens are used (note that we
 * repeat KEY and VALUE here):
 *
 *      BLOCK-SEQUENCE-START
 *      BLOCK-MAPPING-START
 *      BLOCK-END
 *      BLOCK-ENTRY
 *      KEY
 *      VALUE
 *
 * The tokens BLOCK-SEQUENCE-START and BLOCK-MAPPING-START denote indentation
 * increase that precedes a block collection (cf. the INDENT token in Python).
 * The token BLOCK-END denote indentation decrease that ends a block collection
 * (cf. the DEDENT token in Python).  However YAML has some syntax pecularities
 * that makes detections of these tokens more complex.
 *
 * The tokens BLOCK-ENTRY, KEY, and VALUE are used to represent the indicators
 * '-', '?', and ':' correspondingly.
 *
 * The following examples show how the tokens BLOCK-SEQUENCE-START,
 * BLOCK-MAPPING-START, and BLOCK-END are emitted by the Scanner:
 *
 *      1. Block sequences:
 *
 *          - item 1
 *          - item 2
 *          -
 *            - item 3.1
 *            - item 3.2
 *          -
 *            key 1: value 1
 *            key 2: value 2
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          BLOCK-SEQUENCE-START
 *          BLOCK-ENTRY
 *          SCALAR("item 1",plain)
 *          BLOCK-ENTRY
 *          SCALAR("item 2",plain)
 *          BLOCK-ENTRY
 *          BLOCK-SEQUENCE-START
 *          BLOCK-ENTRY
 *          SCALAR("item 3.1",plain)
 *          BLOCK-ENTRY
 *          SCALAR("item 3.2",plain)
 *          BLOCK-END
 *          BLOCK-ENTRY
 *          BLOCK-MAPPING-START
 *          KEY
 *          SCALAR("key 1",plain)
 *          VALUE
 *          SCALAR("value 1",plain)
 *          KEY
 *          SCALAR("key 2",plain)
 *          VALUE
 *          SCALAR("value 2",plain)
 *          BLOCK-END
 *          BLOCK-END
 *          STREAM-END
 *
 *      2. Block mappings:
 *
 *          a simple key: a value   # The KEY token is produced here.
 *          ? a complex key
 *          : another value
 *          a mapping:
 *            key 1: value 1
 *            key 2: value 2
 *          a sequence:
 *            - item 1
 *            - item 2
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          BLOCK-MAPPING-START
 *          KEY
 *          SCALAR("a simple key",plain)
 *          VALUE
 *          SCALAR("a value",plain)
 *          KEY
 *          SCALAR("a complex key",plain)
 *          VALUE
 *          SCALAR("another value",plain)
 *          KEY
 *          SCALAR("a mapping",plain)
 *          BLOCK-MAPPING-START
 *          KEY
 *          SCALAR("key 1",plain)
 *          VALUE
 *          SCALAR("value 1",plain)
 *          KEY
 *          SCALAR("key 2",plain)
 *          VALUE
 *          SCALAR("value 2",plain)
 *          BLOCK-END
 *          KEY
 *          SCALAR("a sequence",plain)
 *          VALUE
 *          BLOCK-SEQUENCE-START
 *          BLOCK-ENTRY
 *          SCALAR("item 1",plain)
 *          BLOCK-ENTRY
 *          SCALAR("item 2",plain)
 *          BLOCK-END
 *          BLOCK-END
 *          STREAM-END
 *
 * YAML does not always require to start a new block collection from a new
 * line.  If the current line contains only '-', '?', and ':' indicators, a new
 * block collection may start at the current line.  The following examples
 * illustrate this case:
 *
 *      1. Collections in a sequence:
 *
 *          - - item 1
 *            - item 2
 *          - key 1: value 1
 *            key 2: value 2
 *          - ? complex key
 *            : complex value
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          BLOCK-SEQUENCE-START
 *          BLOCK-ENTRY
 *          BLOCK-SEQUENCE-START
 *          BLOCK-ENTRY
 *          SCALAR("item 1",plain)
 *          BLOCK-ENTRY
 *          SCALAR("item 2",plain)
 *          BLOCK-END
 *          BLOCK-ENTRY
 *          BLOCK-MAPPING-START
 *          KEY
 *          SCALAR("key 1",plain)
 *          VALUE
 *          SCALAR("value 1",plain)
 *          KEY
 *          SCALAR("key 2",plain)
 *          VALUE
 *          SCALAR("value 2",plain)
 *          BLOCK-END
 *          BLOCK-ENTRY
 *          BLOCK-MAPPING-START
 *          KEY
 *          SCALAR("complex key")
 *          VALUE
 *          SCALAR("complex value")
 *          BLOCK-END
 *          BLOCK-END
 *          STREAM-END
 *
 *      2. Collections in a mapping:
 *
 *          ? a sequence
 *          : - item 1
 *            - item 2
 *          ? a mapping
 *          : key 1: value 1
 *            key 2: value 2
 *
 *      Tokens:
 *
 *          STREAM-START(utf-8)
 *          BLOCK-MAPPING-START
 *          KEY
 *          SCALAR("a sequence",plain)
 *          VALUE
 *          BLOCK-SEQUENCE-START
 *          BLOCK-ENTRY
 *          SCALAR("item 1",plain)
 *          BLOCK-ENTRY
 *          SCALAR("item 2",plain)
 *          BLOCK-END
 *          KEY
 *          SCALAR("a mapping",plain)
 *          VALUE
 *          BLOCK-MAPPING-START
 *          KEY
 *          SCALAR("key 1",plain)
 *          VALUE
 *          SCALAR("value 1",plain)
 *          KEY
 *          SCALAR("key 2",plain)
 *          VALUE
 *          SCALAR("value 2",plain)
 *          BLOCK-END
 *          BLOCK-END
 *          STREAM-END
 *
 * YAML also permits non-indented sequences if they are included into a block
 * mapping.  In this case, the token BLOCK-SEQUENCE-START is not produced:
 *
 *      key:
 *      - item 1    # BLOCK-SEQUENCE-START is NOT produced here.
 *      - item 2
 *
 * Tokens:
 *
 *      STREAM-START(utf-8)
 *      BLOCK-MAPPING-START
 *      KEY
 *      SCALAR("key",plain)
 *      VALUE
 *      BLOCK-ENTRY
 *      SCALAR("item 1",plain)
 *      BLOCK-ENTRY
 *      SCALAR("item 2",plain)
 *      BLOCK-END
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml/yaml.h>

#include <assert.h>

/*
 * Ensure that the buffer contains the required number of characters.
 * Return 1 on success, 0 on failure (reader error or memory error).
 */

#define UPDATE(parser,length)   \
    (parser->unread >= (length) \
        ? 1                     \
        : yaml_parser_update_buffer(parser, (length)))

/*
 * Check the octet at the specified position.
 */

#define CHECK_AT(parser,octet,offset)   \
    (parser->pointer[offset] == (yaml_char_t)(octet))

/*
 * Check the current octet in the buffer.
 */

#define CHECK(parser,octet) CHECK_AT(parser,(octet),0)

/*
 * Check if the character at the specified position is an alphabetical
 * character, a digit, '_', or '-'.
 */

#define IS_ALPHA_AT(parser,offset)  \
     ((parser->pointer[offset] >= (yaml_char_t) '0' &&      \
       parser->pointer[offset] <= (yaml_char_t) '9') ||     \
      (parser->pointer[offset] >= (yaml_char_t) 'A' &&      \
       parser->pointer[offset] <= (yaml_char_t) 'Z') ||     \
      (parser->pointer[offset] >= (yaml_char_t) 'a' &&      \
       parser->pointer[offset] <= (yaml_char_t) 'z') ||     \
      parser->pointer[offset] == '_' ||                     \
      parser->pointer[offset] == '-')

#define IS_ALPHA(parser)    IS_ALPHA_AT(parser,0)

/*
 * Check if the character at the specified position is a digit.
 */

#define IS_DIGIT_AT(parser,offset)  \
     ((parser->pointer[offset] >= (yaml_char_t) '0' &&      \
       parser->pointer[offset] <= (yaml_char_t) '9'))

#define IS_DIGIT(parser)    IS_DIGIT_AT(parser,0)

/*
 * Get the value of a digit.
 */

#define AS_DIGIT_AT(parser,offset)  \
     (parser->pointer[offset] - (yaml_char_t) '0')

#define AS_DIGIT(parser)    AS_DIGIT_AT(parser,0)

/*
 * Check if the character at the specified position is a hex-digit.
 */

#define IS_HEX_AT(parser,offset)    \
     ((parser->pointer[offset] >= (yaml_char_t) '0' &&      \
       parser->pointer[offset] <= (yaml_char_t) '9') ||     \
      (parser->pointer[offset] >= (yaml_char_t) 'A' &&      \
       parser->pointer[offset] <= (yaml_char_t) 'F') ||     \
      (parser->pointer[offset] >= (yaml_char_t) 'a' &&      \
       parser->pointer[offset] <= (yaml_char_t) 'f'))

#define IS_HEX(parser)    IS_HEX_AT(parser,0)

/*
 * Get the value of a hex-digit.
 */

#define AS_HEX_AT(parser,offset)    \
      ((parser->pointer[offset] >= (yaml_char_t) 'A' &&     \
        parser->pointer[offset] <= (yaml_char_t) 'F') ?     \
       (parser->pointer[offset] - (yaml_char_t) 'A' + 10) : \
       (parser->pointer[offset] >= (yaml_char_t) 'a' &&     \
        parser->pointer[offset] <= (yaml_char_t) 'f') ?     \
       (parser->pointer[offset] - (yaml_char_t) 'a' + 10) : \
       (parser->pointer[offset] - (yaml_char_t) '0'))
 
#define AS_HEX(parser)  AS_HEX_AT(parser,0)
 
/*
 * Check if the character at the specified position is NUL.
 */

#define IS_Z_AT(parser,offset)    CHECK_AT(parser,'\0',(offset))

#define IS_Z(parser)    IS_Z_AT(parser,0)

/*
 * Check if the character at the specified position is BOM.
 */

#define IS_BOM_AT(parser,offset)                                    \
     (CHECK_AT(parser,'\xEF',(offset))                              \
      && CHECK_AT(parser,'\xBB',(offset)+1)                         \
      && CHECK_AT(parser,'\xBF',(offset)+1))    /* BOM (#xFEFF) */

#define IS_BOM(parser)  IS_BOM_AT(parser,0)

/*
 * Check if the character at the specified position is space.
 */

#define IS_SPACE_AT(parser,offset)  CHECK_AT(parser,' ',(offset))

#define IS_SPACE(parser)    IS_SPACE_AT(parser,0)

/*
 * Check if the character at the specified position is tab.
 */

#define IS_TAB_AT(parser,offset)    CHECK_AT(parser,'\t',(offset))

#define IS_TAB(parser)  IS_TAB_AT(parser,0)

/*
 * Check if the character at the specified position is blank (space or tab).
 */

#define IS_BLANK_AT(parser,offset)  \
    (IS_SPACE_AT(parser,(offset)) || IS_TAB_AT(parser,(offset)))

#define IS_BLANK(parser)    IS_BLANK_AT(parser,0)

/*
 * Check if the character at the specified position is a line break.
 */

#define IS_BREAK_AT(parser,offset)                                      \
    (CHECK_AT(parser,'\r',(offset))                 /* CR (#xD)*/       \
     || CHECK_AT(parser,'\n',(offset))              /* LF (#xA) */      \
     || (CHECK_AT(parser,'\xC2',(offset))                               \
         && CHECK_AT(parser,'\x85',(offset)+1))     /* NEL (#x85) */    \
     || (CHECK_AT(parser,'\xE2',(offset))                               \
         && CHECK_AT(parser,'\x80',(offset)+1)                          \
         && CHECK_AT(parser,'\xA8',(offset)+2))     /* LS (#x2028) */   \
     || (CHECK_AT(parser,'\xE2',(offset))                               \
         && CHECK_AT(parser,'\x80',(offset)+1)                          \
         && CHECK_AT(parser,'\xA9',(offset)+2)))    /* PS (#x2029) */

#define IS_BREAK(parser)    IS_BREAK_AT(parser,0)

#define IS_CRLF_AT(parser,offset) \
     (CHECK_AT(parser,'\r',(offset)) && CHECK_AT(parser,'\n',(offset)+1))

#define IS_CRLF(parser) IS_CRLF_AT(parser,0)

/*
 * Check if the character is a line break or NUL.
 */

#define IS_BREAKZ_AT(parser,offset) \
    (IS_BREAK_AT(parser,(offset)) || IS_Z_AT(parser,(offset)))

#define IS_BREAKZ(parser)   IS_BREAKZ_AT(parser,0)

/*
 * Check if the character is a line break, space, or NUL.
 */

#define IS_SPACEZ_AT(parser,offset) \
    (IS_SPACE_AT(parser,(offset)) || IS_BREAKZ_AT(parser,(offset)))

#define IS_SPACEZ(parser)   IS_SPACEZ_AT(parser,0)

/*
 * Check if the character is a line break, space, tab, or NUL.
 */

#define IS_BLANKZ_AT(parser,offset) \
    (IS_BLANK_AT(parser,(offset)) || IS_BREAKZ_AT(parser,(offset)))

#define IS_BLANKZ(parser)   IS_BLANKZ_AT(parser,0)

/*
 * Determine the width of the character.
 */

#define WIDTH_AT(parser,offset)                         \
     ((parser->pointer[(offset)] & 0x80) == 0x00 ? 1 :  \
      (parser->pointer[(offset)] & 0xE0) == 0xC0 ? 2 :  \
      (parser->pointer[(offset)] & 0xF0) == 0xE0 ? 3 :  \
      (parser->pointer[(offset)] & 0xF8) == 0xF0 ? 4 : 0)

#define WIDTH(parser)   WIDTH_AT(parser,0)

/*
 * Advance the buffer pointer.
 */

#define FORWARD(parser)                             \
     (parser->index ++,                             \
      parser->column ++,                            \
      parser->unread --,                            \
      parser->pointer += WIDTH(parser))

#define FORWARD_LINE(parser)                        \
     (IS_CRLF(parser) ?                             \
      (parser->index += 2,                          \
       parser->column = 0,                          \
       parser->line ++,                             \
       parser->unread -= 2,                         \
       parser->pointer += 2) :                      \
      IS_BREAK(parser) ?                            \
      (parser->index ++,                            \
       parser->column = 0,                          \
       parser->line ++,                             \
       parser->unread --,                           \
       parser->pointer += WIDTH(parser)) : 0)

/*
 * Resize a string if needed.
 */

#define RESIZE(parser,string)   \
    ((string).pointer-(string).buffer+5 < (string).size ? 1 :   \
     yaml_parser_resize_string(parser, &(string)))

/*
 * Copy a character to a string buffer and advance pointers.
 */

#define COPY(parser,string)     \
     (((*parser->pointer & 0x80) == 0x00 ?                  \
       (*((string).pointer++) = *(parser->pointer++)) :     \
       (*parser->pointer & 0xE0) == 0xC0 ?                  \
       (*((string).pointer++) = *(parser->pointer++),       \
        *((string).pointer++) = *(parser->pointer++)) :     \
       (*parser->pointer & 0xF0) == 0xE0 ?                  \
       (*((string).pointer++) = *(parser->pointer++),       \
        *((string).pointer++) = *(parser->pointer++),       \
        *((string).pointer++) = *(parser->pointer++)) :     \
       (*parser->pointer & 0xF8) == 0xF0 ?                  \
       (*((string).pointer++) = *(parser->pointer++),       \
        *((string).pointer++) = *(parser->pointer++),       \
        *((string).pointer++) = *(parser->pointer++),       \
        *((string).pointer++) = *(parser->pointer++)) : 0), \
      parser->index ++,                                     \
      parser->column ++,                                    \
      parser->unread --)

/*
 * Copy a line break character to a string buffer and advance pointers.
 */

#define COPY_LINE(parser,string)    \
    ((CHECK_AT(parser,'\r',0) && CHECK_AT(parser,'\n',1)) ? /* CR LF -> LF */   \
     (*((string).pointer++) = (yaml_char_t) '\n',                               \
      parser->pointer += 2,                                                     \
      parser->index += 2,                                                       \
      parser->column = 0,                                                       \
      parser->line ++,                                                          \
      parser->unread -= 2) :                                                    \
     (CHECK_AT(parser,'\r',0) || CHECK_AT(parser,'\n',0)) ? /* CR|LF -> LF */   \
     (*((string).pointer++) = (yaml_char_t) '\n',                               \
      parser->pointer ++,                                                       \
      parser->index ++,                                                         \
      parser->column = 0,                                                       \
      parser->line ++,                                                          \
      parser->unread --) :                                                      \
     (CHECK_AT(parser,'\xC2',0) && CHECK_AT(parser,'\x85',1)) ? /* NEL -> LF */ \
     (*((string).pointer++) = (yaml_char_t) '\n',                               \
      parser->pointer += 2,                                                     \
      parser->index ++,                                                         \
      parser->column = 0,                                                       \
      parser->line ++,                                                          \
      parser->unread --) :                                                      \
     (CHECK_AT(parser,'\xE2',0) &&                                              \
      CHECK_AT(parser,'\x80',1) &&                                              \
      (CHECK_AT(parser,'\xA8',2) ||                                             \
       CHECK_AT(parser,'\xA9',2))) ?                    /* LS|PS -> LS|PS */    \
     (*((string).pointer++) = *(parser->pointer++),                             \
      *((string).pointer++) = *(parser->pointer++),                             \
      *((string).pointer++) = *(parser->pointer++),                             \
      parser->index ++,                                                         \
      parser->column = 0,                                                       \
      parser->line ++,                                                          \
      parser->unread --) : 0)

/*
 * Append a string to another string and clear the former string.
 */

#define JOIN(parser,head_string,tail_string)    \
    (yaml_parser_join_string(parser, &(head_string), &(tail_string)) && \
     yaml_parser_clear_string(parser, &(tail_string)))

/*
 * Public API declarations.
 */

YAML_DECLARE(yaml_token_t *)
yaml_parser_get_token(yaml_parser_t *parser);

YAML_DECLARE(yaml_token_t *)
yaml_parser_peek_token(yaml_parser_t *parser);

/*
 * Error handling.
 */

static int
yaml_parser_set_scanner_error(yaml_parser_t *parser, const char *context,
        yaml_mark_t context_mark, const char *problem);

static yaml_mark_t
yaml_parser_get_mark(yaml_parser_t *parser);

/*
 * Buffers and lists.
 */

typedef struct {
    yaml_char_t *buffer;
    yaml_char_t *pointer;
    size_t size;
} yaml_string_t;

static yaml_string_t
yaml_parser_new_string(yaml_parser_t *parser);

static int
yaml_parser_resize_string(yaml_parser_t *parser, yaml_string_t *string);

static int
yaml_parser_join_string(yaml_parser_t *parser,
        yaml_string_t *string1, yaml_string_t *string2);

static int
yaml_parser_clear_string(yaml_parser_t *parser, yaml_string_t *string);

static int
yaml_parser_resize_list(yaml_parser_t *parser, void **buffer, size_t *size,
        size_t item_size);

/*
 * High-level token API.
 */

static int
yaml_parser_fetch_more_tokens(yaml_parser_t *parser);

static int
yaml_parser_fetch_next_token(yaml_parser_t *parser);

/*
 * Potential simple keys.
 */

static int
yaml_parser_stale_simple_keys(yaml_parser_t *parser);

static int
yaml_parser_save_simple_key(yaml_parser_t *parser);

static int
yaml_parser_remove_simple_key(yaml_parser_t *parser);

static int
yaml_parser_increase_flow_level(yaml_parser_t *parser);

static int
yaml_parser_decrease_flow_level(yaml_parser_t *parser);

/*
 * Token manipulation.
 */

static int
yaml_parser_append_token(yaml_parser_t *parser, yaml_token_t *token);

static int
yaml_parser_insert_token(yaml_parser_t *parser,
        int number, yaml_token_t *token);

/*
 * Indentation treatment.
 */

static int
yaml_parser_roll_indent(yaml_parser_t *parser, int column,
        int number, yaml_token_type_t type, yaml_mark_t mark);

static int
yaml_parser_unroll_indent(yaml_parser_t *parser, int column);

/*
 * Token fetchers.
 */

static int
yaml_parser_fetch_stream_start(yaml_parser_t *parser);

static int
yaml_parser_fetch_stream_end(yaml_parser_t *parser);

static int
yaml_parser_fetch_directive(yaml_parser_t *parser);

static int
yaml_parser_fetch_document_indicator(yaml_parser_t *parser,
        yaml_token_type_t type);

static int
yaml_parser_fetch_flow_collection_start(yaml_parser_t *parser,
        yaml_token_type_t type);

static int
yaml_parser_fetch_flow_collection_end(yaml_parser_t *parser,
        yaml_token_type_t type);

static int
yaml_parser_fetch_flow_entry(yaml_parser_t *parser);

static int
yaml_parser_fetch_block_entry(yaml_parser_t *parser);

static int
yaml_parser_fetch_key(yaml_parser_t *parser);

static int
yaml_parser_fetch_value(yaml_parser_t *parser);

static int
yaml_parser_fetch_anchor(yaml_parser_t *parser, yaml_token_type_t type);

static int
yaml_parser_fetch_tag(yaml_parser_t *parser);

static int
yaml_parser_fetch_block_scalar(yaml_parser_t *parser, int literal);

static int
yaml_parser_fetch_flow_scalar(yaml_parser_t *parser, int single);

static int
yaml_parser_fetch_plain_scalar(yaml_parser_t *parser);

/*
 * Token scanners.
 */

static int
yaml_parser_scan_to_next_token(yaml_parser_t *parser);

static yaml_token_t *
yaml_parser_scan_directive(yaml_parser_t *parser);

static int
yaml_parser_scan_directive_name(yaml_parser_t *parser,
        yaml_mark_t start_mark, yaml_char_t **name);

static int
yaml_parser_scan_version_directive_value(yaml_parser_t *parser,
        yaml_mark_t start_mark, int *major, int *minor);

static int
yaml_parser_scan_version_directive_number(yaml_parser_t *parser,
        yaml_mark_t start_mark, int *number);

static int
yaml_parser_scan_tag_directive_value(yaml_parser_t *parser,
        yaml_mark_t mark, yaml_char_t **handle, yaml_char_t **prefix);

static yaml_token_t *
yaml_parser_scan_anchor(yaml_parser_t *parser,
        yaml_token_type_t type);

static yaml_token_t *
yaml_parser_scan_tag(yaml_parser_t *parser);

static int
yaml_parser_scan_tag_handle(yaml_parser_t *parser, int directive,
        yaml_mark_t start_mark, yaml_char_t **handle);

static int
yaml_parser_scan_tag_uri(yaml_parser_t *parser, int directive,
        yaml_char_t *head, yaml_mark_t start_mark, yaml_char_t **uri);

static int
yaml_parser_scan_uri_escapes(yaml_parser_t *parser, int directive,
        yaml_mark_t start_mark, yaml_string_t *string);

static yaml_token_t *
yaml_parser_scan_block_scalar(yaml_parser_t *parser, int literal);

static int
yaml_parser_scan_block_scalar_breaks(yaml_parser_t *parser,
        int *indent, yaml_string_t *breaks,
        yaml_mark_t start_mark, yaml_mark_t *end_mark);

static yaml_token_t *
yaml_parser_scan_flow_scalar(yaml_parser_t *parser, int single);

static yaml_token_t *
yaml_parser_scan_plain_scalar(yaml_parser_t *parser);

/*
 * Get the next token and remove it from the tokens queue.
 */

YAML_DECLARE(yaml_token_t *)
yaml_parser_get_token(yaml_parser_t *parser)
{
    yaml_token_t *token;

    assert(parser); /* Non-NULL parser object is expected. */
    assert(!parser->stream_end_produced);   /* No tokens after STREAM-END. */

    /* Ensure that the tokens queue contains enough tokens. */

    if (!yaml_parser_fetch_more_tokens(parser)) return NULL;

    /* Fetch the next token from the queue. */

    token = parser->tokens[parser->tokens_head];

    /* Move the queue head. */

    parser->tokens[parser->tokens_head++] = NULL;

    parser->tokens_parsed++;

    if (token->type == YAML_STREAM_END_TOKEN) {
        parser->stream_end_produced = 1;
    }

    return token;
}

/*
 * Get the next token, but don't remove it from the queue.
 */

YAML_DECLARE(yaml_token_t *)
yaml_parser_peek_token(yaml_parser_t *parser)
{
    assert(parser); /* Non-NULL parser object is expected. */
    assert(!parser->stream_end_produced);   /* No tokens after STREAM-END. */

    /* Ensure that the tokens queue contains enough tokens. */

    if (!yaml_parser_fetch_more_tokens(parser)) return NULL;

    /* Fetch the next token from the queue. */

    return parser->tokens[parser->tokens_head];
}

/*
 * Create a new string.
 */

static yaml_string_t
yaml_parser_new_string(yaml_parser_t *parser)
{
    yaml_string_t string = { NULL, NULL, 0 };

    string.buffer = yaml_malloc(YAML_DEFAULT_SIZE);
    if (!string.buffer) {
        parser->error = YAML_MEMORY_ERROR;
        return string;
    }

    memset(string.buffer, 0, YAML_DEFAULT_SIZE);
    string.pointer = string.buffer;
    string.size = YAML_DEFAULT_SIZE;

    return string;
}

/*
 * Double the size of a string.
 */

static int
yaml_parser_resize_string(yaml_parser_t *parser, yaml_string_t *string)
{
    yaml_char_t *new_buffer = yaml_realloc(string->buffer, string->size*2);

    if (!new_buffer) {
        yaml_free(string->buffer);
        string->buffer = NULL;
        string->pointer = NULL;
        string->size = 0;
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    memset(new_buffer+string->size, 0, string->size);

    string->pointer = new_buffer + (string->pointer-string->buffer);
    string->buffer = new_buffer;
    string->size *= 2;

    return 1;
}

/*
 * Append a string to another string.
 */

static int
yaml_parser_join_string(yaml_parser_t *parser,
        yaml_string_t *string1, yaml_string_t *string2)
{
    if (string2->buffer == string2->pointer) return 1;

    while (string1->pointer - string1->buffer + string2->pointer - string2->buffer + 1
            > string1->size) {
        if (!yaml_parser_resize_string(parser, string1)) return 0;
    }

    memcpy(string1->pointer, string2->buffer, string2->pointer-string2->buffer);
    string1->pointer += string2->pointer-string2->buffer;

    return 1;
}

/*
 * Fill the string with NULs and move the pointer to the beginning.
 */

static int
yaml_parser_clear_string(yaml_parser_t *parser, yaml_string_t *string)
{
    if (string->buffer == string->pointer) return 1;

    memset(string->buffer, 0, string->pointer-string->buffer);

    string->pointer = string->buffer;

    return 1;
}

/*
 * Double a list.
 */

static int
yaml_parser_resize_list(yaml_parser_t *parser, void **buffer, size_t *size,
        size_t item_size)
{
    void *new_buffer = yaml_realloc(*buffer, item_size*(*size)*2);

    if (!new_buffer) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    memset(new_buffer+item_size*(*size), 0, item_size*(*size));

    *buffer = new_buffer;
    *size *= 2;

    return 1;
}

/*
 * Set the scanner error and return 0.
 */

static int
yaml_parser_set_scanner_error(yaml_parser_t *parser, const char *context,
        yaml_mark_t context_mark, const char *problem)
{
    parser->error = YAML_SCANNER_ERROR;
    parser->context = context;
    parser->context_mark = context_mark;
    parser->problem = problem;
    parser->problem_mark = yaml_parser_get_mark(parser);

    return 0;
}

/*
 * Get the mark for the current buffer position.
 */

static yaml_mark_t
yaml_parser_get_mark(yaml_parser_t *parser)
{
    yaml_mark_t mark = { parser->index, parser->line, parser->column };

    return mark;
}


/*
 * Ensure that the tokens queue contains at least one token which can be
 * returned to the Parser.
 */

static int
yaml_parser_fetch_more_tokens(yaml_parser_t *parser)
{
    int need_more_tokens;
    int k;

    /* While we need more tokens to fetch, do it. */

    while (1)
    {
        /*
         * Check if we really need to fetch more tokens.
         */

        need_more_tokens = 0;

        if (parser->tokens_head == parser->tokens_tail)
        {
            /* Queue is empty. */

            need_more_tokens = 1;
        }
        else
        {
            /* Check if any potential simple key may occupy the head position. */

            if (!yaml_parser_stale_simple_keys(parser))
                return 0;

            for (k = 0; k <= parser->flow_level; k++) {
                yaml_simple_key_t *simple_key = parser->simple_keys[k];
                if (simple_key
                        && (simple_key->token_number == parser->tokens_parsed)) {
                    need_more_tokens = 1;
                    break;
                }
            }
        }

        /* We are finished. */

        if (!need_more_tokens)
            break;

        /* Fetch the next token. */

        if (!yaml_parser_fetch_next_token(parser))
            return 0;
    }

    return 1;
}

/*
 * The dispatcher for token fetchers.
 */

static int
yaml_parser_fetch_next_token(yaml_parser_t *parser)
{
    /* Ensure that the buffer is initialized. */

    if (!UPDATE(parser, 1))
        return 0;

    /* Check if we just started scanning.  Fetch STREAM-START then. */

    if (!parser->stream_start_produced)
        return yaml_parser_fetch_stream_start(parser);

    /* Eat whitespaces and comments until we reach the next token. */

    if (!yaml_parser_scan_to_next_token(parser))
        return 0;

    /* Remove obsolete potential simple keys. */

    if (!yaml_parser_stale_simple_keys(parser))
        return 0;

    /* Check the indentation level against the current column. */

    if (!yaml_parser_unroll_indent(parser, parser->column))
        return 0;

    /*
     * Ensure that the buffer contains at least 4 characters.  4 is the length
     * of the longest indicators ('--- ' and '... ').
     */

    if (!UPDATE(parser, 4))
        return 0;

    /* Is it the end of the stream? */

    if (IS_Z(parser))
        return yaml_parser_fetch_stream_end(parser);

    /* Is it a directive? */

    if (parser->column == 0 && CHECK(parser, '%'))
        return yaml_parser_fetch_directive(parser);

    /* Is it the document start indicator? */

    if (parser->column == 0
            && CHECK_AT(parser, '-', 0)
            && CHECK_AT(parser, '-', 1)
            && CHECK_AT(parser, '-', 2)
            && IS_BLANKZ_AT(parser, 3))
        return yaml_parser_fetch_document_indicator(parser,
                YAML_DOCUMENT_START_TOKEN);

    /* Is it the document end indicator? */

    if (parser->column == 0
            && CHECK_AT(parser, '.', 0)
            && CHECK_AT(parser, '.', 1)
            && CHECK_AT(parser, '.', 2)
            && IS_BLANKZ_AT(parser, 3))
        return yaml_parser_fetch_document_indicator(parser,
                YAML_DOCUMENT_END_TOKEN);

    /* Is it the flow sequence start indicator? */

    if (CHECK(parser, '['))
        return yaml_parser_fetch_flow_collection_start(parser,
                YAML_FLOW_SEQUENCE_START_TOKEN);

    /* Is it the flow mapping start indicator? */

    if (CHECK(parser, '{'))
        return yaml_parser_fetch_flow_collection_start(parser,
                YAML_FLOW_MAPPING_START_TOKEN);

    /* Is it the flow sequence end indicator? */

    if (CHECK(parser, ']'))
        return yaml_parser_fetch_flow_collection_end(parser,
                YAML_FLOW_SEQUENCE_END_TOKEN);

    /* Is it the flow mapping end indicator? */

    if (CHECK(parser, '}'))
        return yaml_parser_fetch_flow_collection_end(parser,
                YAML_FLOW_MAPPING_END_TOKEN);

    /* Is it the flow entry indicator? */

    if (CHECK(parser, ','))
        return yaml_parser_fetch_flow_entry(parser);

    /* Is it the block entry indicator? */

    if (CHECK(parser, '-') && IS_BLANKZ_AT(parser, 1))
        return yaml_parser_fetch_block_entry(parser);

    /* Is it the key indicator? */

    if (CHECK(parser, '?') && (parser->flow_level || IS_BLANKZ_AT(parser, 1)))
        return yaml_parser_fetch_key(parser);

    /* Is it the value indicator? */

    if (CHECK(parser, ':') && (parser->flow_level || IS_BLANKZ_AT(parser, 1)))
        return yaml_parser_fetch_value(parser);

    /* Is it an alias? */

    if (CHECK(parser, '*'))
        return yaml_parser_fetch_anchor(parser, YAML_ALIAS_TOKEN);

    /* Is it an anchor? */

    if (CHECK(parser, '&'))
        return yaml_parser_fetch_anchor(parser, YAML_ANCHOR_TOKEN);

    /* Is it a tag? */

    if (CHECK(parser, '!'))
        return yaml_parser_fetch_tag(parser);

    /* Is it a literal scalar? */

    if (CHECK(parser, '|') && !parser->flow_level)
        return yaml_parser_fetch_block_scalar(parser, 1);

    /* Is it a folded scalar? */

    if (CHECK(parser, '>') && !parser->flow_level)
        return yaml_parser_fetch_block_scalar(parser, 0);

    /* Is it a single-quoted scalar? */

    if (CHECK(parser, '\''))
        return yaml_parser_fetch_flow_scalar(parser, 1);

    /* Is it a double-quoted scalar? */

    if (CHECK(parser, '"'))
        return yaml_parser_fetch_flow_scalar(parser, 0);

    /*
     * Is it a plain scalar?
     *
     * A plain scalar may start with any non-blank characters except
     *
     *      '-', '?', ':', ',', '[', ']', '{', '}',
     *      '#', '&', '*', '!', '|', '>', '\'', '\"',
     *      '%', '@', '`'.
     *
     * In the block context (and, for the '-' indicator, in the flow context
     * too), it may also start with the characters
     *
     *      '-', '?', ':'
     *
     * if it is followed by a non-space character.
     *
     * The last rule is more restrictive than the specification requires.
     */

    if (!(IS_BLANKZ(parser) || CHECK(parser, '-') || CHECK(parser, '?')
                || CHECK(parser, ':') || CHECK(parser, ',') || CHECK(parser, '[')
                || CHECK(parser, ']') || CHECK(parser, '{') || CHECK(parser, '}')
                || CHECK(parser, '#') || CHECK(parser, '&') || CHECK(parser, '*')
                || CHECK(parser, '!') || CHECK(parser, '|') || CHECK(parser, '>')
                || CHECK(parser, '\'') || CHECK(parser, '"') || CHECK(parser, '%')
                || CHECK(parser, '@') || CHECK(parser, '`')) ||
            (CHECK(parser, '-') && !IS_BLANK_AT(parser, 1)) ||
            (!parser->flow_level &&
             (CHECK(parser, '?') || CHECK(parser, ':')) && !IS_BLANKZ_AT(parser, 1)))
        return yaml_parser_fetch_plain_scalar(parser);

    /*
     * If we don't determine the token type so far, it is an error.
     */

    return yaml_parser_set_scanner_error(parser, "while scanning for the next token",
            yaml_parser_get_mark(parser), "found character that cannot start any token");
}

/*
 * Check the list of potential simple keys and remove the positions that
 * cannot contain simple keys anymore.
 */

static int
yaml_parser_stale_simple_keys(yaml_parser_t *parser)
{
    int level;

    /* Check for a potential simple key for each flow level. */

    for (level = 0; level <= parser->flow_level; level++)
    {
        yaml_simple_key_t *simple_key = parser->simple_keys[level];

        /*
         * The specification requires that a simple key
         *
         *  - is limited to a single line,
         *  - is shorter than 1024 characters.
         */

        if (simple_key && (simple_key->line < parser->line ||
                    simple_key->index+1024 < parser->index)) {

            /* Check if the potential simple key to be removed is required. */

            if (simple_key->required) {
                return yaml_parser_set_scanner_error(parser,
                        "while scanning a simple key", simple_key->mark,
                        "could not found expected ':'");
            }

            yaml_free(simple_key);
            parser->simple_keys[level] = NULL;
        }
    }

    return 1;
}

/*
 * Check if a simple key may start at the current position and add it if
 * needed.
 */

static int
yaml_parser_save_simple_key(yaml_parser_t *parser)
{
    /*
     * A simple key is required at the current position if the scanner is in
     * the block context and the current column coincides with the indentation
     * level.
     */

    int required = (!parser->flow_level && parser->indent == parser->column);

    /*
     * A simple key is required only when it is the first token in the current
     * line.  Therefore it is always allowed.  But we add a check anyway.
     */

    assert(parser->simple_key_allowed || !required);    /* Impossible. */

    /*
     * If the current position may start a simple key, save it.
     */

    if (parser->simple_key_allowed)
    {
        yaml_simple_key_t simple_key = { required,
            parser->tokens_parsed + parser->tokens_tail - parser->tokens_head,
            parser->index, parser->line, parser->column,
            yaml_parser_get_mark(parser) };

        if (!yaml_parser_remove_simple_key(parser)) return 0;

        parser->simple_keys[parser->flow_level] =
            yaml_malloc(sizeof(yaml_simple_key_t));
        if (!parser->simple_keys[parser->flow_level]) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }

        *(parser->simple_keys[parser->flow_level]) = simple_key;
    }

    return 1;
}

/*
 * Remove a potential simple key at the current flow level.
 */

static int
yaml_parser_remove_simple_key(yaml_parser_t *parser)
{
    yaml_simple_key_t *simple_key = parser->simple_keys[parser->flow_level];

    if (simple_key)
    {
        /* If the key is required, it is an error. */

        if (simple_key->required) {
            return yaml_parser_set_scanner_error(parser,
                    "while scanning a simple key", simple_key->mark,
                    "could not found expected ':'");
        }

        /* Remove the key from the list. */

        yaml_free(simple_key);
        parser->simple_keys[parser->flow_level] = NULL;
    }

    return 1;
}

/*
 * Increase the flow level and resize the simple key list if needed.
 */

static int
yaml_parser_increase_flow_level(yaml_parser_t *parser)
{
    /* Check if we need to resize the list. */

    if (parser->flow_level == parser->simple_keys_size-1) {
        if (!yaml_parser_resize_list(parser, (void **)&parser->simple_keys,
                    &parser->simple_keys_size, sizeof(yaml_simple_key_t *)))
            return 0;
    }

    /* Increase the flow level and reset the simple key. */

    parser->simple_keys[++parser->flow_level] = NULL;

    return 1;
}

/*
 * Decrease the flow level.
 */

static int
yaml_parser_decrease_flow_level(yaml_parser_t *parser)
{
    assert(parser->flow_level);                         /* Greater than 0. */
    assert(!parser->simple_keys[parser->flow_level]);   /* Must be removed. */

    parser->flow_level --;

    return 1;
}

/*
 * Add a token to the tail of the tokens queue.
 */

static int
yaml_parser_append_token(yaml_parser_t *parser, yaml_token_t *token)
{
    return yaml_parser_insert_token(parser, -1, token);
}

/*
 * Insert the token into the tokens queue.  The number parameter is the
 * ordinal number of the token.  If the number is equal to -1, add the token
 * to the tail of the queue.
 */

static int
yaml_parser_insert_token(yaml_parser_t *parser,
        int number, yaml_token_t *token)
{
    /* The index of the token in the queue. */

    int index = (number == -1)
            ? parser->tokens_tail - parser->tokens_head
            : number - parser->tokens_parsed;

    assert(index >= 0 && index <= (parser->tokens_tail-parser->tokens_head));

    /* Check if we need to resize the queue. */

    if (parser->tokens_head == 0 && parser->tokens_tail == parser->tokens_size) {
        if (!yaml_parser_resize_list(parser, (void **)&parser->tokens,
                    &parser->tokens_size, sizeof(yaml_token_t *)))
            return 0;
    }

    /* Check if we need to move the queue to the beginning of the buffer. */

    if (parser->tokens_tail == parser->tokens_size)
    {
        if (parser->tokens_head < parser->tokens_tail) {
            memmove(parser->tokens, parser->tokens+parser->tokens_head,
                    sizeof(yaml_token_t *)*(parser->tokens_tail-parser->tokens_head));
        }
        parser->tokens_tail -= parser->tokens_head;
        parser->tokens_head = 0;
    }

    /* Check if we need to free space within the queue. */

    if (index < (parser->tokens_tail-parser->tokens_head)) {
        memmove(parser->tokens+parser->tokens_head+index+1,
                parser->tokens+parser->tokens_head+index,
                sizeof(yaml_token_t *)*(parser->tokens_tail-parser->tokens_head-index));
    }

    /* Insert the token. */

    parser->tokens[parser->tokens_head+index] = token;
    parser->tokens_tail ++;

    return 1;
}

/*
 * Push the current indentation level to the stack and set the new level
 * the current column is greater than the indentation level.  In this case,
 * append or insert the specified token into the token queue.
 * 
 */

static int
yaml_parser_roll_indent(yaml_parser_t *parser, int column,
        int number, yaml_token_type_t type, yaml_mark_t mark)
{
    yaml_token_t *token;

    /* In the flow context, do nothing. */

    if (parser->flow_level)
        return 1;

    if (parser->indent < column)
    {
        /* Check if we need to expand the indents stack. */

        if (parser->indents_length == parser->indents_size) {
            if (!yaml_parser_resize_list(parser, (void **)&parser->indents,
                        &parser->indents_size, sizeof(int)))
                return 0;
        }

        /*
         * Push the current indentation level to the stack and set the new
         * indentation level.
         */

        parser->indents[parser->indents_length++] = parser->indent;
        parser->indent = column;

        /* Create a token. */

        token = yaml_token_new(type, mark, mark);
        if (!token) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }

        /* Insert the token into the queue. */

        if (!yaml_parser_insert_token(parser, number, token)) {
            yaml_token_delete(token);
            return 0;
        }
    }

    return 1;
}

/*
 * Pop indentation levels from the indents stack until the current level
 * becomes less or equal to the column.  For each intendation level, append
 * the BLOCK-END token.
 */


static int
yaml_parser_unroll_indent(yaml_parser_t *parser, int column)
{
    yaml_token_t *token;

    /* In the flow context, do nothing. */

    if (parser->flow_level)
        return 1;

    /* Loop through the intendation levels in the stack. */

    while (parser->indent > column)
    {
        yaml_mark_t mark = yaml_parser_get_mark(parser);

        /* Create a token. */

        token = yaml_token_new(YAML_BLOCK_END_TOKEN, mark, mark);
        if (!token) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }

        /* Append the token to the queue. */

        if (!yaml_parser_append_token(parser, token)) {
            yaml_token_delete(token);
            return 0;
        }

        /* Pop the indentation level. */

        assert(parser->indents_length);     /* Non-empty stack expected. */

        parser->indent = parser->indents[--parser->indents_length];
    }

    return 1;
}

/*
 * Initialize the scanner and produce the STREAM-START token.
 */

static int
yaml_parser_fetch_stream_start(yaml_parser_t *parser)
{
    yaml_mark_t mark = yaml_parser_get_mark(parser);
    yaml_token_t *token;

    /* Set the initial indentation. */

    parser->indent = -1;

    /* A simple key is allowed at the beginning of the stream. */

    parser->simple_key_allowed = 1;

    /* We have started. */

    parser->stream_start_produced = 1;

    /* Create the STREAM-START token. */

    token = yaml_stream_start_token_new(parser->encoding, mark, mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the STREAM-END token and shut down the scanner.
 */

static int
yaml_parser_fetch_stream_end(yaml_parser_t *parser)
{
    yaml_mark_t mark = yaml_parser_get_mark(parser);
    yaml_token_t *token;

    /* Reset the indentation level. */

    if (!yaml_parser_unroll_indent(parser, -1))
        return 0;

    /* Reset simple keys. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    parser->simple_key_allowed = 0;

    /* Create the STREAM-END token. */

    token = yaml_stream_end_token_new(mark, mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the YAML-DIRECTIVE or TAG-DIRECTIVE token.
 */

static int
yaml_parser_fetch_directive(yaml_parser_t *parser)
{
    yaml_token_t *token;

    /* Reset the indentation level. */

    if (!yaml_parser_unroll_indent(parser, -1))
        return 0;

    /* Reset simple keys. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    parser->simple_key_allowed = 0;

    /* Create the YAML-DIRECTIVE or TAG-DIRECTIVE token. */

    token = yaml_parser_scan_directive(parser);
    if (!token) return 0;

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the DOCUMENT-START or DOCUMENT-END token.
 */

static int
yaml_parser_fetch_document_indicator(yaml_parser_t *parser,
        yaml_token_type_t type)
{
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token;

    /* Reset the indentation level. */

    if (!yaml_parser_unroll_indent(parser, -1))
        return 0;

    /* Reset simple keys. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    parser->simple_key_allowed = 0;

    /* Consume the token. */

    start_mark = yaml_parser_get_mark(parser);

    FORWARD(parser);
    FORWARD(parser);
    FORWARD(parser);

    end_mark = yaml_parser_get_mark(parser);

    /* Create the DOCUMENT-START or DOCUMENT-END token. */

    token = yaml_token_new(type, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the FLOW-SEQUENCE-START or FLOW-MAPPING-START token.
 */

static int
yaml_parser_fetch_flow_collection_start(yaml_parser_t *parser,
        yaml_token_type_t type)
{
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token;

    /* The indicators '[' and '{' may start a simple key. */

    if (!yaml_parser_save_simple_key(parser))
        return 0;

    /* Increase the flow level. */

    if (!yaml_parser_increase_flow_level(parser))
        return 0;

    /* A simple key may follow the indicators '[' and '{'. */

    parser->simple_key_allowed = 1;

    /* Consume the token. */

    start_mark = yaml_parser_get_mark(parser);
    FORWARD(parser);
    end_mark = yaml_parser_get_mark(parser);

    /* Create the FLOW-SEQUENCE-START of FLOW-MAPPING-START token. */

    token = yaml_token_new(type, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the FLOW-SEQUENCE-END or FLOW-MAPPING-END token.
 */

static int
yaml_parser_fetch_flow_collection_end(yaml_parser_t *parser,
        yaml_token_type_t type)
{
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token;

    /* Reset any potential simple key on the current flow level. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    /* Decrease the flow level. */

    if (!yaml_parser_decrease_flow_level(parser))
        return 0;

    /* No simple keys after the indicators ']' and '}'. */

    parser->simple_key_allowed = 0;

    /* Consume the token. */

    start_mark = yaml_parser_get_mark(parser);
    FORWARD(parser);
    end_mark = yaml_parser_get_mark(parser);

    /* Create the FLOW-SEQUENCE-END of FLOW-MAPPING-END token. */

    token = yaml_token_new(type, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the FLOW-ENTRY token.
 */

static int
yaml_parser_fetch_flow_entry(yaml_parser_t *parser)
{
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token;

    /* Reset any potential simple keys on the current flow level. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    /* Simple keys are allowed after ','. */

    parser->simple_key_allowed = 1;

    /* Consume the token. */

    start_mark = yaml_parser_get_mark(parser);
    FORWARD(parser);
    end_mark = yaml_parser_get_mark(parser);

    /* Create the FLOW-ENTRY token. */

    token = yaml_token_new(YAML_FLOW_ENTRY_TOKEN, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the BLOCK-ENTRY token.
 */

static int
yaml_parser_fetch_block_entry(yaml_parser_t *parser)
{
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token;

    /* Check if the scanner is in the block context. */

    if (!parser->flow_level)
    {
        /* Check if we are allowed to start a new entry. */

        if (!parser->simple_key_allowed) {
            return yaml_parser_set_scanner_error(parser, NULL,
                    yaml_parser_get_mark(parser),
                    "block sequence entries are not allowed in this context");
        }

        /* Add the BLOCK-SEQUENCE-START token if needed. */

        if (!yaml_parser_roll_indent(parser, parser->column, -1,
                    YAML_BLOCK_SEQUENCE_START_TOKEN, yaml_parser_get_mark(parser)))
            return 0;
    }
    else
    {
        /*
         * It is an error for the '-' indicator to occur in the flow context,
         * but we let the Parser detect and report about it because the Parser
         * is able to point to the context.
         */
    }

    /* Reset any potential simple keys on the current flow level. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    /* Simple keys are allowed after '-'. */

    parser->simple_key_allowed = 1;

    /* Consume the token. */

    start_mark = yaml_parser_get_mark(parser);
    FORWARD(parser);
    end_mark = yaml_parser_get_mark(parser);

    /* Create the BLOCK-ENTRY token. */

    token = yaml_token_new(YAML_BLOCK_ENTRY_TOKEN, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the KEY token.
 */

static int
yaml_parser_fetch_key(yaml_parser_t *parser)
{
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token;

    /* In the block context, additional checks are required. */

    if (!parser->flow_level)
    {
        /* Check if we are allowed to start a new key (not nessesary simple). */

        if (!parser->simple_key_allowed) {
            return yaml_parser_set_scanner_error(parser, NULL,
                    yaml_parser_get_mark(parser),
                    "mapping keys are not allowed in this context");
        }

        /* Add the BLOCK-MAPPING-START token if needed. */

        if (!yaml_parser_roll_indent(parser, parser->column, -1,
                    YAML_BLOCK_MAPPING_START_TOKEN, yaml_parser_get_mark(parser)))
            return 0;
    }

    /* Reset any potential simple keys on the current flow level. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    /* Simple keys are allowed after '?' in the block context. */

    parser->simple_key_allowed = (!parser->flow_level);

    /* Consume the token. */

    start_mark = yaml_parser_get_mark(parser);
    FORWARD(parser);
    end_mark = yaml_parser_get_mark(parser);

    /* Create the KEY token. */

    token = yaml_token_new(YAML_KEY_TOKEN, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the VALUE token.
 */

static int
yaml_parser_fetch_value(yaml_parser_t *parser)
{
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token;

    /* Have we found a simple key? */

    if (parser->simple_keys[parser->flow_level])
    {
        yaml_simple_key_t *simple_key = parser->simple_keys[parser->flow_level];

        /* Create the KEY token. */

        token = yaml_token_new(YAML_KEY_TOKEN, simple_key->mark, simple_key->mark);
        if (!token) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }

        /* Insert the token into the queue. */

        if (!yaml_parser_insert_token(parser, simple_key->token_number, token)) {
            yaml_token_delete(token);
            return 0;
        }

        /* In the block context, we may need to add the BLOCK-MAPPING-START token. */

        if (!yaml_parser_roll_indent(parser, simple_key->column,
                    simple_key->token_number,
                    YAML_BLOCK_MAPPING_START_TOKEN, simple_key->mark))
            return 0;

        /* Remove the simple key from the list. */

        yaml_free(simple_key);
        parser->simple_keys[parser->flow_level] = NULL;

        /* A simple key cannot follow another simple key. */

        parser->simple_key_allowed = 0;
    }
    else
    {
        /* The ':' indicator follows a complex key. */

        /* In the block context, extra checks are required. */

        if (!parser->flow_level)
        {
            /* Check if we are allowed to start a complex value. */

            if (!parser->simple_key_allowed) {
                return yaml_parser_set_scanner_error(parser, NULL,
                        yaml_parser_get_mark(parser),
                        "mapping values are not allowed in this context");
            }

            /* Add the BLOCK-MAPPING-START token if needed. */

            if (!yaml_parser_roll_indent(parser, parser->column, -1,
                        YAML_BLOCK_MAPPING_START_TOKEN, yaml_parser_get_mark(parser)))
                return 0;
        }

        /* Simple keys after ':' are allowed in the block context. */

        parser->simple_key_allowed = (!parser->flow_level);
    }

    /* Consume the token. */

    start_mark = yaml_parser_get_mark(parser);
    FORWARD(parser);
    end_mark = yaml_parser_get_mark(parser);

    /* Create the VALUE token. */

    token = yaml_token_new(YAML_VALUE_TOKEN, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the ALIAS or ANCHOR token.
 */

static int
yaml_parser_fetch_anchor(yaml_parser_t *parser, yaml_token_type_t type)
{
    yaml_token_t *token;

    /* An anchor or an alias could be a simple key. */

    if (!yaml_parser_save_simple_key(parser))
        return 0;

    /* A simple key cannot follow an anchor or an alias. */

    parser->simple_key_allowed = 0;

    /* Create the ALIAS or ANCHOR token. */

    token = yaml_parser_scan_anchor(parser, type);
    if (!token) return 0;

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the TAG token.
 */

static int
yaml_parser_fetch_tag(yaml_parser_t *parser)
{
    yaml_token_t *token;

    /* A tag could be a simple key. */

    if (!yaml_parser_save_simple_key(parser))
        return 0;

    /* A simple key cannot follow a tag. */

    parser->simple_key_allowed = 0;

    /* Create the TAG token. */

    token = yaml_parser_scan_tag(parser);
    if (!token) return 0;

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the SCALAR(...,literal) or SCALAR(...,folded) tokens.
 */

static int
yaml_parser_fetch_block_scalar(yaml_parser_t *parser, int literal)
{
    yaml_token_t *token;

    /* Remove any potential simple keys. */

    if (!yaml_parser_remove_simple_key(parser))
        return 0;

    /* A simple key may follow a block scalar. */

    parser->simple_key_allowed = 1;

    /* Create the SCALAR token. */

    token = yaml_parser_scan_block_scalar(parser, literal);
    if (!token) return 0;

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the SCALAR(...,single-quoted) or SCALAR(...,double-quoted) tokens.
 */

static int
yaml_parser_fetch_flow_scalar(yaml_parser_t *parser, int single)
{
    yaml_token_t *token;

    /* A plain scalar could be a simple key. */

    if (!yaml_parser_save_simple_key(parser))
        return 0;

    /* A simple key cannot follow a flow scalar. */

    parser->simple_key_allowed = 0;

    /* Create the SCALAR token. */

    token = yaml_parser_scan_flow_scalar(parser, single);
    if (!token) return 0;

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Produce the SCALAR(...,plain) token.
 */

static int
yaml_parser_fetch_plain_scalar(yaml_parser_t *parser)
{
    yaml_token_t *token;

    /* A plain scalar could be a simple key. */

    if (!yaml_parser_save_simple_key(parser))
        return 0;

    /* A simple key cannot follow a flow scalar. */

    parser->simple_key_allowed = 0;

    /* Create the SCALAR token. */

    token = yaml_parser_scan_plain_scalar(parser);
    if (!token) return 0;

    /* Append the token to the queue. */

    if (!yaml_parser_append_token(parser, token)) {
        yaml_token_delete(token);
        return 0;
    }

    return 1;
}

/*
 * Eat whitespaces and comments until the next token is found.
 */

static int
yaml_parser_scan_to_next_token(yaml_parser_t *parser)
{
    /* Until the next token is not found. */

    while (1)
    {
        /* Allow the BOM mark to start a line. */

        if (!UPDATE(parser, 1)) return 0;

        if (parser->column == 0 && IS_BOM(parser))
            FORWARD(parser);

        /*
         * Eat whitespaces.
         *
         * Tabs are allowed:
         *
         *  - in the flow context;
         *  - in the block context, but not at the beginning of the line or
         *  after '-', '?', or ':' (complex value).  
         */

        if (!UPDATE(parser, 1)) return 0;

        while (CHECK(parser,' ') ||
                ((parser->flow_level || !parser->simple_key_allowed) &&
                 CHECK(parser, '\t'))) {
            FORWARD(parser);
            if (!UPDATE(parser, 1)) return 0;
        }

        /* Eat a comment until a line break. */

        if (CHECK(parser, '#')) {
            while (!IS_BREAKZ(parser)) {
                FORWARD(parser);
                if (!UPDATE(parser, 1)) return 0;
            }
        }

        /* If it is a line break, eat it. */

        if (IS_BREAK(parser))
        {
            if (!UPDATE(parser, 2)) return 0;
            FORWARD_LINE(parser);

            /* In the block context, a new line may start a simple key. */

            if (!parser->flow_level) {
                parser->simple_key_allowed = 1;
            }
        }
        else
        {
            /* We have found a token. */

            break;
        }
    }

    return 1;
}

/*
 * Scan a YAML-DIRECTIVE or TAG-DIRECTIVE token.
 *
 * Scope:
 *      %YAML    1.1    # a comment \n
 *      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *      %TAG    !yaml!  tag:yaml.org,2002:  \n
 *      ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */

static yaml_token_t *
yaml_parser_scan_directive(yaml_parser_t *parser)
{
    yaml_mark_t start_mark, end_mark;
    yaml_char_t *name = NULL;
    int major, minor;
    yaml_char_t *handle = NULL, *prefix = NULL;
    yaml_token_t *token = NULL;

    /* Eat '%'. */

    start_mark = yaml_parser_get_mark(parser);

    FORWARD(parser);

    /* Scan the directive name. */

    if (!yaml_parser_scan_directive_name(parser, start_mark, &name))
        goto error;

    /* Is it a YAML directive? */

    if (strcmp((char *)name, "YAML") == 0)
    {
        /* Scan the VERSION directive value. */

        if (!yaml_parser_scan_version_directive_value(parser, start_mark,
                    &major, &minor))
            goto error;

        end_mark = yaml_parser_get_mark(parser);

        /* Create a VERSION-DIRECTIVE token. */

        token = yaml_version_directive_token_new(major, minor,
                start_mark, end_mark);
        if (!token) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }
    }

    /* Is it a TAG directive? */

    else if (strcmp((char *)name, "TAG") == 0)
    {
        /* Scan the TAG directive value. */

        if (!yaml_parser_scan_tag_directive_value(parser, start_mark,
                    &handle, &prefix))
            goto error;

        end_mark = yaml_parser_get_mark(parser);

        /* Create a TAG-DIRECTIVE token. */

        token = yaml_tag_directive_token_new(handle, prefix,
                start_mark, end_mark);
        if (!token) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }
    }

    /* Unknown directive. */

    else
    {
        yaml_parser_set_scanner_error(parser, "while scanning a directive",
                start_mark, "found uknown directive name");
        goto error;
    }

    /* Eat the rest of the line including any comments. */

    while (IS_BLANK(parser)) {
        FORWARD(parser);
        if (!UPDATE(parser, 1)) goto error;
    }

    if (CHECK(parser, '#')) {
        while (!IS_BREAKZ(parser)) {
            FORWARD(parser);
            if (!UPDATE(parser, 1)) goto error;
        }
    }

    /* Check if we are at the end of the line. */

    if (!IS_BREAKZ(parser)) {
        yaml_parser_set_scanner_error(parser, "while scanning a directive",
                start_mark, "did not found expected comment or line break");
        goto error;
    }

    /* Eat a line break. */

    if (IS_BREAK(parser)) {
        if (!UPDATE(parser, 2)) goto error;
        FORWARD_LINE(parser);
    }

    yaml_free(name);

    return token;

error:
    yaml_free(token);
    yaml_free(prefix);
    yaml_free(handle);
    yaml_free(name);
    return NULL;
}

/*
 * Scan the directive name.
 *
 * Scope:
 *      %YAML   1.1     # a comment \n
 *       ^^^^
 *      %TAG    !yaml!  tag:yaml.org,2002:  \n
 *       ^^^
 */

static int
yaml_parser_scan_directive_name(yaml_parser_t *parser,
        yaml_mark_t start_mark, yaml_char_t **name)
{
    yaml_string_t string = yaml_parser_new_string(parser);

    if (!string.buffer) goto error;

    /* Consume the directive name. */

    if (!UPDATE(parser, 1)) goto error;

    while (IS_ALPHA(parser))
    {
        if (!RESIZE(parser, string)) goto error;
        COPY(parser, string);
        if (!UPDATE(parser, 1)) goto error;
    }

    /* Check if the name is empty. */

    if (string.buffer == string.pointer) {
        yaml_parser_set_scanner_error(parser, "while scanning a directive",
                start_mark, "cannot found expected directive name");
        goto error;
    }

    /* Check for an blank character after the name. */

    if (!IS_BLANKZ(parser)) {
        yaml_parser_set_scanner_error(parser, "while scanning a directive",
                start_mark, "found unexpected non-alphabetical character");
        goto error;
    }

    *name = string.buffer;

    return 1;

error:
    yaml_free(string.buffer);
    return 0;
}

/*
 * Scan the value of VERSION-DIRECTIVE.
 *
 * Scope:
 *      %YAML   1.1     # a comment \n
 *           ^^^^^^
 */

static int
yaml_parser_scan_version_directive_value(yaml_parser_t *parser,
        yaml_mark_t start_mark, int *major, int *minor)
{
    /* Eat whitespaces. */

    if (!UPDATE(parser, 1)) return 0;

    while (IS_BLANK(parser)) {
        FORWARD(parser);
        if (!UPDATE(parser, 1)) return 0;
    }

    /* Consume the major version number. */

    if (!yaml_parser_scan_version_directive_number(parser, start_mark, major))
        return 0;

    /* Eat '.'. */

    if (!CHECK(parser, '.')) {
        return yaml_parser_set_scanner_error(parser, "while scanning a %YAML directive",
                start_mark, "did not find expected digit or '.' character");
    }

    FORWARD(parser);

    /* Consume the minor version number. */

    if (!yaml_parser_scan_version_directive_number(parser, start_mark, minor))
        return 0;
}

#define MAX_NUMBER_LENGTH   9

/*
 * Scan the version number of VERSION-DIRECTIVE.
 *
 * Scope:
 *      %YAML   1.1     # a comment \n
 *              ^
 *      %YAML   1.1     # a comment \n
 *                ^
 */

static int
yaml_parser_scan_version_directive_number(yaml_parser_t *parser,
        yaml_mark_t start_mark, int *number)
{
    int value = 0;
    size_t length = 0;

    /* Repeat while the next character is digit. */

    if (!UPDATE(parser, 1)) return 0;

    while (IS_DIGIT(parser))
    {
        /* Check if the number is too long. */

        if (++length > MAX_NUMBER_LENGTH) {
            return yaml_parser_set_scanner_error(parser, "while scanning a %YAML directive",
                    start_mark, "found extremely long version number");
        }

        value = value*10 + AS_DIGIT(parser);

        FORWARD(parser);

        if (!UPDATE(parser, 1)) return 0;
    }

    /* Check if the number was present. */

    if (!length) {
        return yaml_parser_set_scanner_error(parser, "while scanning a %YAML directive",
                start_mark, "did not find expected version number");
    }

    *number = value;

    return 1;
}

/*
 * Scan the value of a TAG-DIRECTIVE token.
 *
 * Scope:
 *      %TAG    !yaml!  tag:yaml.org,2002:  \n
 *          ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 */

static int
yaml_parser_scan_tag_directive_value(yaml_parser_t *parser,
        yaml_mark_t start_mark, yaml_char_t **handle, yaml_char_t **prefix)
{
    yaml_char_t *handle_value = NULL;
    yaml_char_t *prefix_value = NULL;

    /* Eat whitespaces. */

    if (!UPDATE(parser, 1)) goto error;

    while (IS_BLANK(parser)) {
        FORWARD(parser);
        if (!UPDATE(parser, 1)) goto error;
    }

    /* Scan a handle. */

    if (!yaml_parser_scan_tag_handle(parser, 1, start_mark, &handle_value))
        goto error;

    /* Expect a whitespace. */

    if (!UPDATE(parser, 1)) goto error;

    if (!IS_BLANK(parser)) {
        yaml_parser_set_scanner_error(parser, "while scanning a %TAG directive",
                start_mark, "did not find expected whitespace");
        goto error;
    }

    /* Eat whitespaces. */

    while (IS_BLANK(parser)) {
        FORWARD(parser);
        if (!UPDATE(parser, 1)) goto error;
    }

    /* Scan a prefix. */

    if (!yaml_parser_scan_tag_uri(parser, 1, NULL, start_mark, &prefix_value))
        goto error;

    /* Expect a whitespace or line break. */

    if (!UPDATE(parser, 1)) goto error;

    if (!IS_BLANKZ(parser)) {
        yaml_parser_set_scanner_error(parser, "while scanning a %TAG directive",
                start_mark, "did not find expected whitespace or line break");
        goto error;
    }

    *handle = handle_value;
    *prefix = prefix_value;

    return 1;

error:
    yaml_free(handle_value);
    yaml_free(prefix_value);
    return 0;
}

static yaml_token_t *
yaml_parser_scan_anchor(yaml_parser_t *parser,
        yaml_token_type_t type)
{
    int length = 0;
    yaml_mark_t start_mark, end_mark;
    yaml_token_t *token = NULL;
    yaml_string_t string = yaml_parser_new_string(parser);

    if (!string.buffer) goto error;

    /* Eat the indicator character. */

    start_mark = yaml_parser_get_mark(parser);

    FORWARD(parser);

    /* Consume the value. */

    if (!UPDATE(parser, 1)) goto error;

    while (IS_ALPHA(parser)) {
        if (!RESIZE(parser, string)) goto error;
        COPY(parser, string);
        if (!UPDATE(parser, 1)) goto error;
        length ++;
    }

    end_mark = yaml_parser_get_mark(parser);

    /*
     * Check if length of the anchor is greater than 0 and it is followed by
     * a whitespace character or one of the indicators:
     *
     *      '?', ':', ',', ']', '}', '%', '@', '`'.
     */

    if (!length || !(IS_BLANKZ(parser) || CHECK(parser, '?') || CHECK(parser, ':') ||
                CHECK(parser, ',') || CHECK(parser, ']') || CHECK(parser, '}') ||
                CHECK(parser, '%') || CHECK(parser, '@') || CHECK(parser, '`'))) {
        yaml_parser_set_scanner_error(parser, type == YAML_ANCHOR_TOKEN ?
                "while scanning an anchor" : "while scanning an alias", start_mark,
                "did not find expected alphabetic or numeric character");
        goto error;
    }

    /* Create a token. */

    token = type == YAML_ANCHOR_TOKEN ?
        yaml_anchor_token_new(string.buffer, start_mark, end_mark) :
        yaml_alias_token_new(string.buffer, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    return token;

error:
    yaml_free(string.buffer);
    yaml_free(token);
    return 0;
}

/*
 * Scan a TAG token.
 */

static yaml_token_t *
yaml_parser_scan_tag(yaml_parser_t *parser)
{
    yaml_char_t *handle = NULL;
    yaml_char_t *suffix = NULL;
    yaml_token_t *token = NULL;
    yaml_mark_t start_mark, end_mark;

    start_mark = yaml_parser_get_mark(parser);

    /* Check if the tag is in the canonical form. */

    if (!UPDATE(parser, 2)) goto error;

    if (CHECK_AT(parser, '<', 1))
    {
        /* Set the handle to '' */

        handle = yaml_malloc(1);
        if (!handle) goto error;
        handle[0] = '\0';

        /* Eat '!<' */

        FORWARD(parser);
        FORWARD(parser);

        /* Consume the tag value. */

        if (!yaml_parser_scan_tag_uri(parser, 0, NULL, start_mark, &suffix))
            goto error;

        /* Check for '>' and eat it. */

        if (!CHECK(parser, '>')) {
            yaml_parser_set_scanner_error(parser, "while scanning a tag",
                    start_mark, "did not find the expected '>'");
            goto error;
        }

        FORWARD(parser);
    }
    else
    {
        /* The tag has either the '!suffix' or the '!handle!suffix' form. */

        /* First, try to scan a handle. */

        if (!yaml_parser_scan_tag_handle(parser, 0, start_mark, &handle))
            goto error;

        /* Check if it is, indeed, handle. */

        if (handle[0] == '!' && handle[1] != '\0' && handle[strlen((char *)handle)-1] == '!')
        {
            /* Scan the suffix now. */

            if (!yaml_parser_scan_tag_uri(parser, 0, NULL, start_mark, &suffix))
                goto error;
        }
        else
        {
            /* It wasn't a handle after all.  Scan the rest of the tag. */

            if (!yaml_parser_scan_tag_uri(parser, 0, handle, start_mark, &suffix))
                goto error;

            /* Set the handle to '!'. */

            yaml_free(handle);
            handle = yaml_malloc(2);
            if (!handle) goto error;
            handle[0] = '!';
            handle[1] = '\0';

            /*
             * A special case: the '!' tag.
             */

            if (suffix[0] == '\0') {
                yaml_char_t *tmp = handle;
                handle = suffix;
                suffix = tmp;
            }
        }
    }

    /* Check the character which ends the tag. */

    if (!UPDATE(parser, 1)) goto error;

    if (!IS_BLANKZ(parser)) {
        yaml_parser_set_scanner_error(parser, "while scanning a tag",
                start_mark, "did not found expected whitespace or line break");
        goto error;
    }

    end_mark = yaml_parser_get_mark(parser);

    /* Create a token. */

    token = yaml_tag_token_new(handle, suffix, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    return token;

error:
    yaml_free(handle);
    yaml_free(suffix);
    return NULL;
}

/*
 * Scan a tag handle.
 */

static int
yaml_parser_scan_tag_handle(yaml_parser_t *parser, int directive,
        yaml_mark_t start_mark, yaml_char_t **handle)
{
    yaml_string_t string = yaml_parser_new_string(parser);

    if (!string.buffer) goto error;

    /* Check the initial '!' character. */

    if (!UPDATE(parser, 1)) goto error;

    if (!CHECK(parser, '!')) {
        yaml_parser_set_scanner_error(parser, directive ?
                "while scanning a tag directive" : "while scanning a tag",
                start_mark, "did not find expected '!'");
        goto error;
    }

    /* Copy the '!' character. */

    COPY(parser, string);

    /* Copy all subsequent alphabetical and numerical characters. */

    if (!UPDATE(parser, 1)) goto error;

    while (IS_ALPHA(parser))
    {
        if (!RESIZE(parser, string)) goto error;
        COPY(parser, string);
        if (!UPDATE(parser, 1)) goto error;
    }

    /* Check if the trailing character is '!' and copy it. */

    if (CHECK(parser, '!'))
    {
        if (!RESIZE(parser, string)) goto error;
        COPY(parser, string);
    }
    else
    {
        /*
         * It's either the '!' tag or not really a tag handle.  If it's a %TAG
         * directive, it's an error.  If it's a tag token, it must be a part of
         * URI.
         */

        if (directive && !(string.buffer[0] == '!' && string.buffer[1] == '\0')) {
            yaml_parser_set_scanner_error(parser, "while parsing a tag directive",
                    start_mark, "did not find expected '!'");
            goto error;
        }
    }

    *handle = string.buffer;

    return 1;

error:
    yaml_free(string.buffer);
    return 0;
}

/*
 * Scan a tag.
 */

static int
yaml_parser_scan_tag_uri(yaml_parser_t *parser, int directive,
        yaml_char_t *head, yaml_mark_t start_mark, yaml_char_t **uri)
{
    size_t length = head ? strlen((char *)head) : 0;
    yaml_string_t string = yaml_parser_new_string(parser);

    if (!string.buffer) goto error;

    /* Resize the string to include the head. */

    while (string.size <= length) {
        if (!yaml_parser_resize_string(parser, &string)) goto error;
    }

    /*
     * Copy the head if needed.
     *
     * Note that we don't copy the leading '!' character.
     */

    if (length > 1) {
        memcpy(string.buffer, head+1, length-1);
        string.pointer += length-1;
    }

    /* Scan the tag. */

    if (!UPDATE(parser, 1)) goto error;

    /*
     * The set of characters that may appear in URI is as follows:
     *
     *      '0'-'9', 'A'-'Z', 'a'-'z', '_', '-', ';', '/', '?', ':', '@', '&',
     *      '=', '+', '$', ',', '.', '!', '~', '*', '\'', '(', ')', '[', ']',
     *      '%'.
     */

    while (IS_ALPHA(parser) || CHECK(parser, ';') || CHECK(parser, '/') ||
            CHECK(parser, '?') || CHECK(parser, ':') || CHECK(parser, '@') ||
            CHECK(parser, '&') || CHECK(parser, '=') || CHECK(parser, '+') ||
            CHECK(parser, '$') || CHECK(parser, ',') || CHECK(parser, '.') ||
            CHECK(parser, '!') || CHECK(parser, '~') || CHECK(parser, '*') ||
            CHECK(parser, '\'') || CHECK(parser, '(') || CHECK(parser, ')') ||
            CHECK(parser, '[') || CHECK(parser, ']') || CHECK(parser, '%'))
    {
        if (!RESIZE(parser, string)) goto error;

        /* Check if it is a URI-escape sequence. */

        if (CHECK(parser, '%')) {
            if (!yaml_parser_scan_uri_escapes(parser,
                        directive, start_mark, &string)) goto error;
        }
        else {
            COPY(parser, string);
        }

        length ++;
        if (!UPDATE(parser, 1)) goto error;
    }

    /* Check if the tag is non-empty. */

    if (!length) {
        yaml_parser_set_scanner_error(parser, directive ?
                "while parsing a %TAG directive" : "while parsing a tag",
                start_mark, "did not find expected tag URI");
        goto error;
    }

    *uri = string.buffer;

    return 1;

error:
    yaml_free(string.buffer);
    return 0;
}

/*
 * Decode an URI-escape sequence corresponding to a single UTF-8 character.
 */

static int
yaml_parser_scan_uri_escapes(yaml_parser_t *parser, int directive,
        yaml_mark_t start_mark, yaml_string_t *string)
{
    int width = 0;

    /* Decode the required number of characters. */

    do {

        unsigned char octet = 0;

        /* Check for a URI-escaped octet. */

        if (!UPDATE(parser, 3)) return 0;

        if (!(CHECK(parser, '%') && IS_HEX_AT(parser, 1) && IS_HEX_AT(parser, 2))) {
            return yaml_parser_set_scanner_error(parser, directive ?
                    "while parsing a %TAG directive" : "while parsing a tag",
                    start_mark, "did not find URI escaped octet");
        }

        /* Get the octet. */

        octet = (AS_HEX_AT(parser, 1) << 4) + AS_HEX_AT(parser, 2);

        /* If it is the leading octet, determine the length of the UTF-8 sequence. */

        if (!width)
        {
            width = (octet & 0x80) == 0x00 ? 1 :
                    (octet & 0xE0) == 0xC0 ? 2 :
                    (octet & 0xF0) == 0xE0 ? 3 :
                    (octet & 0xF8) == 0xF0 ? 4 : 0;
            if (!width) {
                return yaml_parser_set_scanner_error(parser, directive ?
                        "while parsing a %TAG directive" : "while parsing a tag",
                        start_mark, "found an incorrect leading UTF-8 octet");
            }
        }
        else
        {
            /* Check if the trailing octet is correct. */

            if ((octet & 0xC0) != 0x80) {
                return yaml_parser_set_scanner_error(parser, directive ?
                        "while parsing a %TAG directive" : "while parsing a tag",
                        start_mark, "found an incorrect trailing UTF-8 octet");
            }
        }

        /* Copy the octet and move the pointers. */

        *(string->pointer++) = octet;
        FORWARD(parser);
        FORWARD(parser);
        FORWARD(parser);

    } while (--width);

    return 1;
}

/*
 * Scan a block scalar.
 */

static yaml_token_t *
yaml_parser_scan_block_scalar(yaml_parser_t *parser, int literal)
{
    yaml_mark_t start_mark;
    yaml_mark_t end_mark;
    yaml_string_t string = yaml_parser_new_string(parser);
    yaml_string_t leading_break = yaml_parser_new_string(parser);
    yaml_string_t trailing_breaks = yaml_parser_new_string(parser);
    yaml_token_t *token = NULL;
    int chomping = 0;
    int increment = 0;
    int indent = 0;
    int leading_blank = 0;
    int trailing_blank = 0;

    if (!string.buffer) goto error;
    if (!leading_break.buffer) goto error;
    if (!trailing_breaks.buffer) goto error;

    /* Eat the indicator '|' or '>'. */

    start_mark = yaml_parser_get_mark(parser);

    FORWARD(parser);

    /* Scan the additional block scalar indicators. */

    if (!UPDATE(parser, 1)) goto error;

    /* Check for a chomping indicator. */

    if (CHECK(parser, '+') || CHECK(parser, '-'))
    {
        /* Set the chomping method and eat the indicator. */

        chomping = CHECK(parser, '+') ? +1 : -1;

        FORWARD(parser);

        /* Check for an indentation indicator. */

        if (!UPDATE(parser, 1)) goto error;

        if (IS_DIGIT(parser))
        {
            /* Check that the intendation is greater than 0. */

            if (CHECK(parser, '0')) {
                yaml_parser_set_scanner_error(parser, "while scanning a block scalar",
                        start_mark, "found an intendation indicator equal to 0");
                goto error;
            }

            /* Get the intendation level and eat the indicator. */

            increment = AS_DIGIT(parser);

            FORWARD(parser);
        }
    }

    /* Do the same as above, but in the opposite order. */

    else if (IS_DIGIT(parser))
    {
        if (CHECK(parser, '0')) {
            yaml_parser_set_scanner_error(parser, "while scanning a block scalar",
                    start_mark, "found an intendation indicator equal to 0");
            goto error;
        }

        increment = AS_DIGIT(parser);

        FORWARD(parser);

        if (!UPDATE(parser, 1)) goto error;

        if (CHECK(parser, '+') || CHECK(parser, '-')) {
            chomping = CHECK(parser, '+') ? +1 : -1;
            FORWARD(parser);
        }
    }

    /* Eat whitespaces and comments to the end of the line. */

    if (!UPDATE(parser, 1)) goto error;

    while (IS_BLANK(parser)) {
        FORWARD(parser);
        if (!UPDATE(parser, 1)) goto error;
    }

    if (CHECK(parser, '#')) {
        while (!IS_BREAKZ(parser)) {
            FORWARD(parser);
            if (!UPDATE(parser, 1)) goto error;
        }
    }

    /* Check if we are at the end of the line. */

    if (!IS_BREAKZ(parser)) {
        yaml_parser_set_scanner_error(parser, "while scanning a block scalar",
                start_mark, "did not found expected comment or line break");
        goto error;
    }

    /* Eat a line break. */

    if (IS_BREAK(parser)) {
        if (!UPDATE(parser, 2)) goto error;
        FORWARD_LINE(parser);
    }

    end_mark = yaml_parser_get_mark(parser);

    /* Set the intendation level if it was specified. */

    if (increment) {
        indent = parser->indent >= 0 ? parser->indent+increment : increment;
    }

    /* Scan the leading line breaks and determine the indentation level if needed. */

    if (!yaml_parser_scan_block_scalar_breaks(parser, &indent, &trailing_breaks,
                start_mark, &end_mark)) goto error;

    /* Scan the block scalar content. */

    if (!UPDATE(parser, 1)) goto error;

    while (parser->column == indent && !IS_Z(parser))
    {
        /*
         * We are at the beginning of a non-empty line.
         */

        /* Is it a trailing whitespace? */

        trailing_blank = IS_BLANK(parser);

        /* Check if we need to fold the leading line break. */

        if (!literal && (*leading_break.buffer == '\n')
                && !leading_blank && !trailing_blank)
        {
            /* Do we need to join the lines by space? */

            if (*trailing_breaks.buffer == '\0') {
                if (!RESIZE(parser, string)) goto error;
                *(string.pointer ++) = ' ';
            }

            yaml_parser_clear_string(parser, &leading_break);
        }
        else {
            if (!JOIN(parser, string, leading_break)) goto error;
        }

        /* Append the remaining line breaks. */

        if (!JOIN(parser, string, trailing_breaks)) goto error;

        /* Is it a leading whitespace? */

        leading_blank = IS_BLANK(parser);

        /* Consume the current line. */

        while (!IS_BREAKZ(parser)) {
            if (!RESIZE(parser, string)) goto error;
            COPY(parser, string);
            if (!UPDATE(parser, 1)) goto error;
        }

        /* Consume the line break. */

        if (!UPDATE(parser, 2)) goto error;

        COPY_LINE(parser, leading_break);

        /* Eat the following intendation spaces and line breaks. */

        if (!yaml_parser_scan_block_scalar_breaks(parser,
                    &indent, &trailing_breaks, start_mark, &end_mark)) goto error;
    }

    /* Chomp the tail. */

    if (chomping != -1) {
        if (!JOIN(parser, string, leading_break)) goto error;
    }
    if (chomping == 1) {
        if (!JOIN(parser, string, trailing_breaks)) goto error;
    }

    /* Create a token. */

    token = yaml_scalar_token_new(string.buffer, string.pointer-string.buffer,
            literal ? YAML_LITERAL_SCALAR_STYLE : YAML_FOLDED_SCALAR_STYLE,
            start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    yaml_free(leading_break.buffer);
    yaml_free(trailing_breaks.buffer);

    return token;

error:
    yaml_free(string.buffer);
    yaml_free(leading_break.buffer);
    yaml_free(trailing_breaks.buffer);

    return NULL;
}

/*
 * Scan intendation spaces and line breaks for a block scalar.  Determine the
 * intendation level if needed.
 */

static int
yaml_parser_scan_block_scalar_breaks(yaml_parser_t *parser,
        int *indent, yaml_string_t *breaks,
        yaml_mark_t start_mark, yaml_mark_t *end_mark)
{
    int max_indent = 0;

    *end_mark = yaml_parser_get_mark(parser);

    /* Eat the intendation spaces and line breaks. */

    while (1)
    {
        /* Eat the intendation spaces. */

        if (!UPDATE(parser, 1)) return 0;

        while ((!*indent || parser->column < *indent) && IS_SPACE(parser)) {
            FORWARD(parser);
            if (!UPDATE(parser, 1)) return 0;
        }

        if (parser->column > max_indent)
            max_indent = parser->column;

        /* Check for a tab character messing the intendation. */

        if ((!*indent || parser->column < *indent) && IS_TAB(parser)) {
            return yaml_parser_set_scanner_error(parser, "while scanning a block scalar",
                    start_mark, "found a tab character where an intendation space is expected");
        }

        /* Have we found a non-empty line? */

        if (!IS_BREAK(parser)) break;

        /* Consume the line break. */

        if (!UPDATE(parser, 2)) return 0;
        if (!RESIZE(parser, *breaks)) return 0;
        COPY_LINE(parser, *breaks);
        *end_mark = yaml_parser_get_mark(parser);
    }

    /* Determine the indentation level if needed. */

    if (!*indent) {
        *indent = max_indent;
        if (*indent < parser->indent + 1)
            *indent = parser->indent + 1;
        if (*indent < 1)
            *indent = 1;
    }

   return 1; 
}

/*
 * Scan a quoted scalar.
 */

static yaml_token_t *
yaml_parser_scan_flow_scalar(yaml_parser_t *parser, int single)
{
    yaml_mark_t start_mark;
    yaml_mark_t end_mark;
    yaml_string_t string = yaml_parser_new_string(parser);
    yaml_string_t leading_break = yaml_parser_new_string(parser);
    yaml_string_t trailing_breaks = yaml_parser_new_string(parser);
    yaml_string_t whitespaces = yaml_parser_new_string(parser);
    yaml_token_t *token = NULL;
    int leading_blanks;

    if (!string.buffer) goto error;
    if (!leading_break.buffer) goto error;
    if (!trailing_breaks.buffer) goto error;
    if (!whitespaces.buffer) goto error;

    /* Eat the left quote. */

    start_mark = yaml_parser_get_mark(parser);

    FORWARD(parser);

    /* Consume the content of the quoted scalar. */

    while (1)
    {
        /* Check that there are no document indicators at the beginning of the line. */

        if (!UPDATE(parser, 4)) goto error;

        if (parser->column == 0 &&
            ((CHECK_AT(parser, '-', 0) &&
              CHECK_AT(parser, '-', 1) &&
              CHECK_AT(parser, '-', 2)) ||
             (CHECK_AT(parser, '.', 0) &&
              CHECK_AT(parser, '.', 1) &&
              CHECK_AT(parser, '.', 2))) &&
            IS_BLANKZ_AT(parser, 3))
        {
            yaml_parser_set_scanner_error(parser, "while scanning a quoted scalar",
                    start_mark, "found unexpected document indicator");
            goto error;
        }

        /* Check for EOF. */

        if (IS_Z(parser)) {
            yaml_parser_set_scanner_error(parser, "while scanning a quoted scalar",
                    start_mark, "found unexpected end of stream");
            goto error;
        }

        /* Consume non-blank characters. */

        if (!UPDATE(parser, 2)) goto error;
        if (!RESIZE(parser, string)) goto error;

        leading_blanks = 0;

        while (!IS_BLANKZ(parser))
        {
            /* Check for an escaped single quote. */

            if (single && CHECK_AT(parser, '\'', 0) && CHECK_AT(parser, '\'', 1))
            {
                *(string.pointer++) = '\'';
                FORWARD(parser);
                FORWARD(parser);
            }

            /* Check for the right quote. */

            else if (CHECK(parser, single ? '\'' : '"'))
            {
                break;
            }

            /* Check for an escaped line break. */

            else if (!single && CHECK(parser, '\\') && IS_BREAK_AT(parser, 1))
            {
                if (!UPDATE(parser, 3)) goto error;
                FORWARD(parser);
                FORWARD_LINE(parser);
                leading_blanks = 1;
                break;
            }

            /* Check for an escape sequence. */

            else if (!single && CHECK(parser, '\\'))
            {
                int code_length = 0;

                /* Check the escape character. */

                switch (parser->pointer[1])
                {
                    case '0':
                        *(string.pointer++) = '\0';
                        break;

                    case 'a':
                        *(string.pointer++) = '\x07';
                        break;

                    case 'b':
                        *(string.pointer++) = '\x08';
                        break;

                    case 't':
                    case '\t':
                        *(string.pointer++) = '\x09';
                        break;

                    case 'n':
                        *(string.pointer++) = '\x0A';
                        break;

                    case 'v':
                        *(string.pointer++) = '\x0B';
                        break;

                    case 'f':
                        *(string.pointer++) = '\x0C';
                        break;

                    case 'r':
                        *(string.pointer++) = '\x0D';
                        break;

                    case 'e':
                        *(string.pointer++) = '\x1B';
                        break;

                    case ' ':
                        *(string.pointer++) = '\x20';
                        break;

                    case '"':
                        *(string.pointer++) = '"';
                        break;

                    case '\'':
                        *(string.pointer++) = '\'';
                        break;

                    case '\\':
                        *(string.pointer++) = '\\';
                        break;

                    case 'N':   /* NEL (#x85) */
                        *(string.pointer++) = '\xC2';
                        *(string.pointer++) = '\x85';
                        break;

                    case '_':   /* #xA0 */
                        *(string.pointer++) = '\xC2';
                        *(string.pointer++) = '\xA0';
                        break;

                    case 'L':   /* LS (#x2028) */
                        *(string.pointer++) = '\xE2';
                        *(string.pointer++) = '\x80';
                        *(string.pointer++) = '\xA8';
                        break;

                    case 'P':   /* PS (#x2029) */
                        *(string.pointer++) = '\xE2';
                        *(string.pointer++) = '\x80';
                        *(string.pointer++) = '\xA9';
                        break;

                    case 'x':
                        code_length = 2;
                        break;

                    case 'u':
                        code_length = 4;
                        break;

                    case 'U':
                        code_length = 8;
                        break;

                    default:
                        yaml_parser_set_scanner_error(parser, "while parsing a quoted scalar",
                                start_mark, "found unknown escape character");
                        goto error;
                }

                FORWARD(parser);
                FORWARD(parser);

                /* Consume an arbitrary escape code. */

                if (code_length)
                {
                    unsigned int value = 0;
                    int k;

                    /* Scan the character value. */

                    if (!UPDATE(parser, code_length)) goto error;

                    for (k = 0; k < code_length; k ++) {
                        if (!IS_HEX_AT(parser, k)) {
                            yaml_parser_set_scanner_error(parser, "while parsing a quoted scalar",
                                    start_mark, "did not find expected hexdecimal number");
                            goto error;
                        }
                        value = (value << 4) + AS_HEX_AT(parser, k);
                    }

                    /* Check the value and write the character. */

                    if ((value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFF) {
                        yaml_parser_set_scanner_error(parser, "while parsing a quoted scalar",
                                start_mark, "found invalid Unicode character escape code");
                        goto error;
                    }

                    if (value <= 0x7F) {
                        *(string.pointer++) = value;
                    }
                    else if (value <= 0x7FF) {
                        *(string.pointer++) = 0xC0 + (value >> 6);
                        *(string.pointer++) = 0x80 + (value & 0x3F);
                    }
                    else if (value <= 0xFFFF) {
                        *(string.pointer++) = 0xE0 + (value >> 12);
                        *(string.pointer++) = 0x80 + ((value >> 6) & 0x3F);
                        *(string.pointer++) = 0x80 + (value & 0x3F);
                    }
                    else {
                        *(string.pointer++) = 0xF0 + (value >> 18);
                        *(string.pointer++) = 0x80 + ((value >> 12) & 0x3F);
                        *(string.pointer++) = 0x80 + ((value >> 6) & 0x3F);
                        *(string.pointer++) = 0x80 + (value & 0x3F);
                    }

                    /* Advance the pointer. */

                    for (k = 0; k < code_length; k ++) {
                        FORWARD(parser);
                    }
                }
            }

            else
            {
                /* It is a non-escaped non-blank character. */

                COPY(parser, string);
            }

            if (!UPDATE(parser, 2)) goto error;
            if (!RESIZE(parser, string)) goto error;
        }

        /* Check if we are at the end of the scalar. */

        if (CHECK(parser, single ? '\'' : '"'))
            break;

        /* Consume blank characters. */

        if (!UPDATE(parser, 1)) goto error;

        while (IS_BLANK(parser) || IS_BREAK(parser))
        {
            if (IS_BLANK(parser))
            {
                /* Consume a space or a tab character. */

                if (!leading_blanks) {
                    if (!RESIZE(parser, whitespaces)) goto error;
                    COPY(parser, whitespaces);
                }
                else {
                    FORWARD(parser);
                }
            }
            else
            {
                if (!UPDATE(parser, 2)) goto error;

                /* Check if it is a first line break. */

                if (!leading_blanks)
                {
                    yaml_parser_clear_string(parser, &whitespaces);
                    COPY_LINE(parser, leading_break);
                    leading_blanks = 1;
                }
                else
                {
                    if (!RESIZE(parser, trailing_breaks)) goto error;
                    COPY_LINE(parser, trailing_breaks);
                }
            }
            if (!UPDATE(parser, 1)) goto error;
        }

        /* Join the whitespaces or fold line breaks. */

        if (!RESIZE(parser, string)) goto error;

        if (leading_blanks)
        {
            /* Do we need to fold line breaks? */

            if (leading_break.buffer[0] == '\n') {
                if (trailing_breaks.buffer[0] == '\0') {
                    *(string.pointer++) = ' ';
                }
                else {
                    if (!JOIN(parser, string, trailing_breaks)) goto error;
                }
                yaml_parser_clear_string(parser, &leading_break);
            }
            else {
                if (!JOIN(parser, string, leading_break)) goto error;
                if (!JOIN(parser, string, trailing_breaks)) goto error;
            }
        }
        else
        {
            if (!JOIN(parser, string, whitespaces)) goto error;
        }
    }

    /* Eat the right quote. */

    FORWARD(parser);

    end_mark = yaml_parser_get_mark(parser);

    /* Create a token. */

    token = yaml_scalar_token_new(string.buffer, string.pointer-string.buffer,
            single ? YAML_SINGLE_QUOTED_SCALAR_STYLE : YAML_DOUBLE_QUOTED_SCALAR_STYLE,
            start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    yaml_free(leading_break.buffer);
    yaml_free(trailing_breaks.buffer);
    yaml_free(whitespaces.buffer);

    return token;

error:
    yaml_free(string.buffer);
    yaml_free(leading_break.buffer);
    yaml_free(trailing_breaks.buffer);
    yaml_free(whitespaces.buffer);

    return NULL;
}

/*
 * Scan a plain scalar.
 */

static yaml_token_t *
yaml_parser_scan_plain_scalar(yaml_parser_t *parser)
{
    yaml_mark_t start_mark;
    yaml_mark_t end_mark;
    yaml_string_t string = yaml_parser_new_string(parser);
    yaml_string_t leading_break = yaml_parser_new_string(parser);
    yaml_string_t trailing_breaks = yaml_parser_new_string(parser);
    yaml_string_t whitespaces = yaml_parser_new_string(parser);
    yaml_token_t *token = NULL;
    int leading_blanks = 0;
    int indent = parser->indent+1;

    if (!string.buffer) goto error;
    if (!leading_break.buffer) goto error;
    if (!trailing_breaks.buffer) goto error;
    if (!whitespaces.buffer) goto error;

    start_mark = yaml_parser_get_mark(parser);

    /* Consume the content of the plain scalar. */

    while (1)
    {
        /* Check for a document indicator. */

        if (!UPDATE(parser, 4)) goto error;

        if (parser->column == 0 &&
            ((CHECK_AT(parser, '-', 0) &&
              CHECK_AT(parser, '-', 1) &&
              CHECK_AT(parser, '-', 2)) ||
             (CHECK_AT(parser, '.', 0) &&
              CHECK_AT(parser, '.', 1) &&
              CHECK_AT(parser, '.', 2))) &&
            IS_BLANKZ_AT(parser, 3)) break;

        /* Check for a comment. */

        if (CHECK(parser, '#'))
            break;

        /* Consume non-blank characters. */

        while (!IS_BLANKZ(parser))
        {
            /* Check for 'x:x' in the flow context. TODO: Fix the test "spec-08-13". */

            if (parser->flow_level && CHECK(parser, ':') && !IS_BLANKZ_AT(parser, 1)) {
                yaml_parser_set_scanner_error(parser, "while scanning a plain scalar",
                        start_mark, "found unexpected ':'");
                goto error;
            }

            /* Check for indicators that may end a plain scalar. */

            if ((CHECK(parser, ':') && IS_BLANKZ_AT(parser, 1)) ||
                    (parser->flow_level &&
                     (CHECK(parser, ',') || CHECK(parser, ':') ||
                      CHECK(parser, '?') || CHECK(parser, '[') ||
                      CHECK(parser, ']') || CHECK(parser, '{') ||
                      CHECK(parser, '}'))))
                break;

            /* Check if we need to join whitespaces and breaks. */

            if (leading_blanks || whitespaces.buffer != whitespaces.pointer)
            {
                if (!RESIZE(parser, string)) goto error;

                if (leading_blanks)
                {
                    /* Do we need to fold line breaks? */

                    if (leading_break.buffer[0] == '\n') {
                        if (trailing_breaks.buffer[0] == '\0') {
                            *(string.pointer++) = ' ';
                        }
                        else {
                            if (!JOIN(parser, string, trailing_breaks)) goto error;
                        }
                        yaml_parser_clear_string(parser, &leading_break);
                    }
                    else {
                        if (!JOIN(parser, string, leading_break)) goto error;
                        if (!JOIN(parser, string, trailing_breaks)) goto error;
                    }

                    leading_blanks = 0;
                }
                else
                {
                    if (!JOIN(parser, string, whitespaces)) goto error;
                }
            }

            /* Copy the character. */

            if (!RESIZE(parser, string)) goto error;

            COPY(parser, string);

            end_mark = yaml_parser_get_mark(parser);

            if (!UPDATE(parser, 2)) goto error;
        }

        /* Is it the end? */

        if (!(IS_BLANK(parser) || IS_BREAK(parser)))
            break;

        /* Consume blank characters. */

        if (!UPDATE(parser, 1)) goto error;

        while (IS_BLANK(parser) || IS_BREAK(parser))
        {
            if (IS_BLANK(parser))
            {
                /* Check for tab character that abuse intendation. */

                if (leading_blanks && parser->column < indent && IS_TAB(parser)) {
                    yaml_parser_set_scanner_error(parser, "while scanning a plain scalar",
                            start_mark, "found a tab character that violate intendation");
                    goto error;
                }

                /* Consume a space or a tab character. */

                if (!leading_blanks) {
                    if (!RESIZE(parser, whitespaces)) goto error;
                    COPY(parser, whitespaces);
                }
                else {
                    FORWARD(parser);
                }
            }
            else
            {
                if (!UPDATE(parser, 2)) goto error;

                /* Check if it is a first line break. */

                if (!leading_blanks)
                {
                    yaml_parser_clear_string(parser, &whitespaces);
                    COPY_LINE(parser, leading_break);
                    leading_blanks = 1;
                }
                else
                {
                    if (!RESIZE(parser, trailing_breaks)) goto error;
                    COPY_LINE(parser, trailing_breaks);
                }
            }
            if (!UPDATE(parser, 1)) goto error;
        }

        /* Check intendation level. */

        if (!parser->flow_level && parser->column < indent)
            break;
    }

    /* Create a token. */

    token = yaml_scalar_token_new(string.buffer, string.pointer-string.buffer,
            YAML_PLAIN_SCALAR_STYLE, start_mark, end_mark);
    if (!token) {
        parser->error = YAML_MEMORY_ERROR;
        return 0;
    }

    /* Note that we change the 'simple_key_allowed' flag. */

    if (leading_blanks) {
        parser->simple_key_allowed = 1;
    }

    yaml_free(leading_break.buffer);
    yaml_free(trailing_breaks.buffer);
    yaml_free(whitespaces.buffer);

    return token;

error:
    yaml_free(string.buffer);
    yaml_free(leading_break.buffer);
    yaml_free(trailing_breaks.buffer);
    yaml_free(whitespaces.buffer);

    return NULL;
}

