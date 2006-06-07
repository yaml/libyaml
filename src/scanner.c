
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
    (parser->buffer[offset] == (yaml_char_t)(octet))

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

/*
 * Indentation treatment.
 */

static int
yaml_parser_roll_indent(yaml_parser_t *parser, int column);

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
yaml_parser_fetch_document_start(yaml_parser_t *parser);

static int
yaml_parser_fetch_document_end(yaml_parser_t *parser);

static int
yaml_parser_fetch_document_indicator(yaml_parser_t *parser,
        yaml_token_type_t type);

static int
yaml_parser_fetch_flow_sequence_start(yaml_parser_t *parser);

static int
yaml_parser_fetch_flow_mapping_start(yaml_parser_t *parser);

static int
yaml_parser_fetch_flow_collection_start(yaml_parser_t *parser,
        yaml_token_type_t type);

static int
yaml_parser_fetch_flow_sequence_end(yaml_parser_t *parser);

static int
yaml_parser_fetch_flow_mapping_end(yaml_parser_t *parser);

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
yaml_parser_fetch_alias(yaml_parser_t *parser);

static int
yaml_parser_fetch_anchor(yaml_parser_t *parser);

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
        return yaml_parser_fetch_document_start(parser);

    /* Is it the document end indicator? */

    if (parser->column == 0
            && CHECK_AT(parser, '.', 0)
            && CHECK_AT(parser, '.', 1)
            && CHECK_AT(parser, '.', 2)
            && IS_BLANKZ_AT(parser, 3))
        return yaml_parser_fetch_document_start(parser);

    /* Is it the flow sequence start indicator? */

    if (CHECK(parser, '['))
        return yaml_parser_fetch_flow_sequence_start(parser);

    /* Is it the flow mapping start indicator? */

    if (CHECK(parser, '{'))
        return yaml_parser_fetch_flow_mapping_start(parser);

    /* Is it the flow sequence end indicator? */

    if (CHECK(parser, ']'))
        return yaml_parser_fetch_flow_sequence_end(parser);

    /* Is it the flow mapping end indicator? */

    if (CHECK(parser, '}'))
        return yaml_parser_fetch_flow_mapping_end(parser);

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
        return yaml_parser_fetch_alias(parser);

    /* Is it an anchor? */

    if (CHECK(parser, '&'))
        return yaml_parser_fetch_anchor(parser);

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

