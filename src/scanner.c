
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
 * Check if the character at the specified position is NUL.
 */

#define IS_Z_AT(parser,offset)    CHECK_AT(parser,'\0',(offset))

#define IS_Z(parser)    IS_Z_AT(parser,0)

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
         && CHECK_AT(parser,'\x85',(offset+1)))     /* NEL (#x85) */    \
     || (CHECK_AT(parser,'\xE2',(offset))                               \
         && CHECK_AT(parser,'\x80',(offset+1))                          \
         && CHECK_AT(parser,'\xA8',(offset+2)))     /* LS (#x2028) */   \
     || (CHECK_AT(parser,'\xE2',(offset))                               \
         && CHECK_AT(parser,'\x80',(offset+1))                          \
         && CHECK_AT(parser,'\xA9',(offset+2))))    /* LS (#x2029) */

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

#define FORWARD(parser) \
     (parser->index ++,                             \
      ((IS_BREAK(parser) && !IS_CRLF(parser)) ?     \
       (parser->line ++, parser->column = 0) :      \
       (parser->column ++)),                        \
      parser->unread --,                            \
      parser->pointer += WIDTH(parser))

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
yaml_parser_scan_yaml_directive_value(yaml_parser_t *parser,
        yaml_mark_t start_mark, int *major, int *minor);

static int
yaml_parser_scan_yaml_directive_number(yaml_parser_t *parser,
        yaml_mark_t start_mark, int *number);

static int
yaml_parser_scan_tag_directive_value(yaml_parser_t *parser,
        yaml_char_t **handle, yaml_char_t **prefix);

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
        yaml_mark_t start_mark, yaml_char_t **url);

static yaml_token_t *
yaml_parser_scan_block_scalar(yaml_parser_t *parser, int literal);

static int
yaml_parser_scan_block_scalar_indicators(yaml_parser_t *parser,
        yaml_mark_t start_mark, int *chomping, int *increment);

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
    if (parser->tokens_head == parser->tokens_size)
        parser->tokens_head = 0;

    parser->tokens_parsed++;

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

    if (CHECK(parser, '?') && (!parser->flow_level || IS_BLANKZ_AT(parser, 1)))
        return yaml_parser_fetch_key(parser);

    /* Is it the value indicator? */

    if (CHECK(parser, ':') && (!parser->flow_level || IS_BLANKZ_AT(parser, 1)))
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
     * In the block context, it may also start with the characters
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
            (!parser->flow_level &&
             (CHECK(parser, '-') || CHECK(parser, '?') || CHECK(parser, ':')) &&
             IS_BLANKZ_AT(parser, 1)))
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
                    simple_key->index < parser->index+1024)) {

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

    if (parser->flow_level == parser->simple_keys_size-1)
    {
        yaml_simple_key_t **new_simple_keys =
                yaml_realloc(parser->simple_keys,
                    sizeof(yaml_simple_key_t *) * parser->simple_keys_size * 2);

        if (!new_simple_keys) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }

        memset(new_simple_keys+parser->simple_keys_size, 0,
                sizeof(yaml_simple_key_t *)*parser->simple_keys_size);

        parser->simple_keys = new_simple_keys;
        parser->simple_keys_size *= 2;
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

    if (parser->tokens_head == 0 && parser->tokens_tail == parser->tokens_size)
    {
        yaml_token_t **new_tokens = yaml_realloc(parser->tokens,
                sizeof(yaml_token_t *) * parser->tokens_size * 2);

        if (!new_tokens) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }

        memset(new_tokens+parser->tokens_size, 0,
                sizeof(yaml_token_t *)*parser->tokens_size);

        parser->tokens = new_tokens;
        parser->tokens_size *= 2;
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

        if (parser->indents_length == parser->indents_size)
        {
            int *new_indents = yaml_realloc(parser->indents,
                    sizeof(int) * parser->indents_size * 2);

            if (!new_indents) {
                parser->error = YAML_MEMORY_ERROR;
                return 0;
            }

            memset(new_indents+parser->indents_size, 0,
                    sizeof(int)*parser->indents_size);

            parser->indents = new_indents;
            parser->indents_size *= 2;
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

    /* We have finished. */

    parser->stream_end_produced = 1;

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

        if (!yaml_parser_roll_indent(parser, parser->column,
                    simple_key->token_number,
                    YAML_BLOCK_MAPPING_START_TOKEN, simple_key->mark))
            return 0;

        /* Remove the simple key from the list. */

        if (!yaml_parser_remove_simple_key(parser)) return 0;

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

        /* Remove a potential simple key from the list. */

        if (!yaml_parser_remove_simple_key(parser)) return 0;

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

