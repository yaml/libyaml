
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml.h>

#include <assert.h>

YAML_DECLARE(const char *)
yaml_get_version_string(void)
{
    return YAML_VERSION_STRING;
}

YAML_DECLARE(void)
yaml_get_version(int *major, int *minor, int *patch)
{
    *major = YAML_VERSION_MAJOR;
    *minor = YAML_VERSION_MINOR;
    *patch = YAML_VERSION_PATCH;
}

/*
 * Allocate a dynamic memory block.
 */

YAML_DECLARE(void *)
yaml_malloc(size_t size)
{
    return malloc(size ? size : 1);
}

/*
 * Reallocate a dynamic memory block.
 */

YAML_DECLARE(void *)
yaml_realloc(void *ptr, size_t size)
{
    return ptr ? realloc(ptr, size ? size : 1) : malloc(size ? size : 1);
}

/*
 * Free a dynamic memory block.
 */

YAML_DECLARE(void)
yaml_free(void *ptr)
{
    if (ptr) free(ptr);
}

/*
 * Create a new parser object.
 */

YAML_DECLARE(yaml_parser_t *)
yaml_parser_new(void)
{
    yaml_parser_t *parser;

    /* Allocate the parser structure. */

    parser = yaml_malloc(sizeof(yaml_parser_t));
    if (!parser) goto error;

    memset(parser, 0, sizeof(yaml_parser_t));

    /* Allocate the raw buffer. */

    parser->raw_buffer = yaml_malloc(YAML_RAW_BUFFER_SIZE);
    if (!parser->raw_buffer) goto error;
    memset(parser->raw_buffer, 0, YAML_RAW_BUFFER_SIZE);

    parser->raw_pointer = parser->raw_buffer;
    parser->raw_unread = 0;

    /* Allocate the character buffer. */

    parser->buffer = yaml_malloc(YAML_BUFFER_SIZE);
    if (!parser->buffer) goto error;
    memset(parser->buffer, 0, YAML_BUFFER_SIZE);

    parser->buffer_end = parser->buffer;
    parser->pointer = parser->buffer;
    parser->unread = 0;

    /* Allocate the tokens queue. */

    parser->tokens = yaml_malloc(YAML_DEFAULT_SIZE*sizeof(yaml_token_t *));
    if (!parser->tokens) goto error;
    memset(parser->tokens, 0, YAML_DEFAULT_SIZE*sizeof(yaml_token_t *));

    parser->tokens_size = YAML_DEFAULT_SIZE;
    parser->tokens_head = 0;
    parser->tokens_tail = 0;
    parser->tokens_parsed = 0;

    /* Allocate the indents stack. */

    parser->indents = yaml_malloc(YAML_DEFAULT_SIZE*sizeof(int));
    if (!parser->indents) goto error;
    memset(parser->indents, 0, YAML_DEFAULT_SIZE*sizeof(int));

    parser->indents_size = YAML_DEFAULT_SIZE;
    parser->indents_length = 0;

    /* Allocate the stack of potential simple keys. */

    parser->simple_keys = yaml_malloc(YAML_DEFAULT_SIZE*sizeof(yaml_simple_key_t *));
    if (!parser->simple_keys) goto error;
    memset(parser->simple_keys, 0, YAML_DEFAULT_SIZE*sizeof(yaml_simple_key_t *));

    parser->simple_keys_size = YAML_DEFAULT_SIZE;

    /* Allocate the stack of parser states. */

    parser->states = yaml_malloc(YAML_DEFAULT_SIZE*sizeof(yaml_parser_state_t));
    if (!parser->states) goto error;
    memset(parser->states, 0, YAML_DEFAULT_SIZE*sizeof(yaml_parser_state_t));

    parser->states_size = YAML_DEFAULT_SIZE;

    /* Set the initial state. */

    parser->state = YAML_PARSE_STREAM_START_STATE;

    /* Allocate the stack of marks. */

    parser->marks = yaml_malloc(YAML_DEFAULT_SIZE*sizeof(yaml_mark_t));
    if (!parser->marks) goto error;
    memset(parser->marks, 0, YAML_DEFAULT_SIZE*sizeof(yaml_mark_t));

    parser->marks_size = YAML_DEFAULT_SIZE;

    /* Allocate the list of TAG directives. */

    parser->tag_directives = yaml_malloc(YAML_DEFAULT_SIZE*sizeof(yaml_tag_directive_t *));
    if (!parser->tag_directives) goto error;
    memset(parser->tag_directives, 0, YAML_DEFAULT_SIZE*sizeof(yaml_tag_directive_t *));

    parser->tag_directives_size = YAML_DEFAULT_SIZE;

    /* Done. */

    return parser;

    /* On error, free allocated buffers. */

error:

    if (!parser) return NULL;

    yaml_free(parser->tag_directives);
    yaml_free(parser->marks);
    yaml_free(parser->states);
    yaml_free(parser->simple_keys);
    yaml_free(parser->indents);
    yaml_free(parser->tokens);
    yaml_free(parser->buffer);
    yaml_free(parser->raw_buffer);

    yaml_free(parser);

    return NULL;
}

/*
 * Destroy a parser object.
 */

YAML_DECLARE(void)
yaml_parser_delete(yaml_parser_t *parser)
{
    assert(parser); /* Non-NULL parser object expected. */

    /*yaml_free(parser->tag_directives);*/
    yaml_free(parser->marks);
    yaml_free(parser->states);
    yaml_free(parser->simple_keys);
    yaml_free(parser->indents);
    yaml_free(parser->tokens);
    yaml_free(parser->buffer);
    yaml_free(parser->raw_buffer);

    memset(parser, 0, sizeof(yaml_parser_t));

    yaml_free(parser);
}

/*
 * String read handler.
 */

static int
yaml_string_read_handler(void *data, unsigned char *buffer, size_t size,
        size_t *size_read)
{
    yaml_string_input_t *input = data;

    if (input->current == input->end) {
        *size_read = 0;
        return 1;
    }

    if (size > (input->end - input->current)) {
        size = input->end - input->current;
    }

    memcpy(buffer, input->current, size);
    input->current += size;
    *size_read = size;
    return 1;
}

/*
 * File read handler.
 */

static int
yaml_file_read_handler(void *data, unsigned char *buffer, size_t size,
        size_t *size_read)
{
    *size_read = fread(buffer, 1, size, (FILE *)data);
    return !ferror((FILE *)data);
}

/*
 * Set a string input.
 */

YAML_DECLARE(void)
yaml_parser_set_input_string(yaml_parser_t *parser,
        unsigned char *input, size_t size)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->read_handler);  /* You can set the source only once. */
    assert(input);  /* Non-NULL input string expected. */

    parser->string_input.start = input;
    parser->string_input.current = input;
    parser->string_input.end = input+size;

    parser->read_handler = yaml_string_read_handler;
    parser->read_handler_data = &parser->string_input;
}

/*
 * Set a file input.
 */

YAML_DECLARE(void)
yaml_parser_set_input_file(yaml_parser_t *parser, FILE *file)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->read_handler);  /* You can set the source only once. */
    assert(file);   /* Non-NULL file object expected. */

    parser->read_handler = yaml_file_read_handler;
    parser->read_handler_data = file;
}

/*
 * Set a generic input.
 */

YAML_DECLARE(void)
yaml_parser_set_input(yaml_parser_t *parser,
        yaml_read_handler_t *handler, void *data)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->read_handler);  /* You can set the source only once. */
    assert(handler);    /* Non-NULL read handler expected. */

    parser->read_handler = handler;
    parser->read_handler_data = data;
}

/*
 * Set the source encoding.
 */

YAML_DECLARE(void)
yaml_parser_set_encoding(yaml_parser_t *parser, yaml_encoding_t encoding)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->encoding); /* Encoding is already set or detected. */

    parser->encoding = encoding;
}

/*
 * Create a token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_token_new(yaml_token_type_t type,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_malloc(sizeof(yaml_token_t));

    if (!token) return NULL;

    memset(token, 0, sizeof(yaml_token_t));

    token->type = type;
    token->start_mark = start_mark;
    token->end_mark = end_mark;

    return token;
}

/*
 * Create a STREAM-START token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_stream_start_token_new(yaml_encoding_t encoding,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_STREAM_START_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.stream_start.encoding = encoding;

    return token;
}

/*
 * Create a STREAM-END token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_stream_end_token_new(yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_STREAM_END_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    return token;
}

/*
 * Create a VERSION-DIRECTIVE token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_version_directive_token_new(int major, int minor,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_VERSION_DIRECTIVE_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.version_directive.major = major;
    token->data.version_directive.minor = minor;

    return token;
}

/*
 * Create a TAG-DIRECTIVE token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_tag_directive_token_new(yaml_char_t *handle, yaml_char_t *prefix,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_TAG_DIRECTIVE_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.tag_directive.handle = handle;
    token->data.tag_directive.prefix = prefix;

    return token;
}

/*
 * Create an ALIAS token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_alias_token_new(yaml_char_t *anchor,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_ALIAS_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.alias.value = anchor;

    return token;
}

/*
 * Create an ANCHOR token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_anchor_token_new(yaml_char_t *anchor,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_ANCHOR_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.anchor.value = anchor;

    return token;
}

/*
 * Create a TAG token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_tag_token_new(yaml_char_t *handle, yaml_char_t *suffix,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_TAG_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.tag.handle = handle;
    token->data.tag.suffix = suffix;

    return token;
}

/*
 * Create a SCALAR token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_scalar_token_new(yaml_char_t *value, size_t length,
        yaml_scalar_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_SCALAR_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.scalar.value = value;
    token->data.scalar.length = length;
    token->data.scalar.style = style;

    return token;
}

/*
 * Destroy a token object.
 */

YAML_DECLARE(void)
yaml_token_delete(yaml_token_t *token)
{
    assert(token);  /* Non-NULL token object expected. */

    switch (token->type)
    {
        case YAML_TAG_DIRECTIVE_TOKEN:
            yaml_free(token->data.tag_directive.handle);
            yaml_free(token->data.tag_directive.prefix);
            break;

        case YAML_ALIAS_TOKEN:
            yaml_free(token->data.alias.value);
            break;

        case YAML_ANCHOR_TOKEN:
            yaml_free(token->data.anchor.value);
            break;

        case YAML_TAG_TOKEN:
            yaml_free(token->data.tag.handle);
            yaml_free(token->data.tag.suffix);
            break;

        case YAML_SCALAR_TOKEN:
            yaml_free(token->data.scalar.value);
            break;
    }

    memset(token, 0, sizeof(yaml_token_t));

    yaml_free(token);
}

/*
 * Create an event.
 */

static yaml_event_t *
yaml_event_new(yaml_event_type_t type,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_malloc(sizeof(yaml_event_t));

    if (!event) return NULL;

    memset(event, 0, sizeof(yaml_event_t));

    event->type = type;
    event->start_mark = start_mark;
    event->end_mark = end_mark;

    return event;
}

/*
 * Create a STREAM-START event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_stream_start_event_new(yaml_encoding_t encoding,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_event_new(YAML_STREAM_START_EVENT,
            start_mark, end_mark);

    if (!event) return NULL;

    event->data.stream_start.encoding = encoding;

    return event;
}

/*
 * Create a STREAM-END event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_stream_end_event_new(yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    return yaml_event_new(YAML_STREAM_END_EVENT, start_mark, end_mark);
}

/*
 * Create a DOCUMENT-START event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_document_start_event_new(yaml_version_directive_t *version_directive,
        yaml_tag_directive_t **tag_directives, int implicit,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_event_new(YAML_DOCUMENT_START_EVENT,
            start_mark, end_mark);

    if (!event) return NULL;

    event->data.document_start.version_directive = version_directive;
    event->data.document_start.tag_directives = tag_directives;
    event->data.document_start.implicit = implicit;

    return event;
}

/*
 * Create a DOCUMENT-END event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_document_end_event_new(int implicit,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_event_new(YAML_DOCUMENT_END_EVENT,
            start_mark, end_mark);

    if (!event) return NULL;

    event->data.document_end.implicit = implicit;

    return event;
}

/*
 * Create an ALIAS event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_alias_event_new(yaml_char_t *anchor,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_event_new(YAML_ALIAS_EVENT,
            start_mark, end_mark);

    if (!event) return NULL;

    event->data.alias.anchor = anchor;

    return event;
}

/*
 * Create a SCALAR event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_scalar_event_new(yaml_char_t *anchor, yaml_char_t *tag,
        yaml_char_t *value, size_t length,
        int plain_implicit, int quoted_implicit,
        yaml_scalar_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_event_new(YAML_SCALAR_EVENT,
            start_mark, end_mark);

    if (!event) return NULL;

    event->data.scalar.anchor = anchor;
    event->data.scalar.tag = tag;
    event->data.scalar.value = value;
    event->data.scalar.length = length;
    event->data.scalar.plain_implicit = plain_implicit;
    event->data.scalar.quoted_implicit = quoted_implicit;
    event->data.scalar.style = style;

    return event;
}

/*
 * Create a SEQUENCE-START event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_sequence_start_event_new(yaml_char_t *anchor, yaml_char_t *tag,
        int implicit, yaml_sequence_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_event_new(YAML_SEQUENCE_START_EVENT,
            start_mark, end_mark);

    if (!event) return NULL;

    event->data.sequence_start.anchor = anchor;
    event->data.sequence_start.tag = tag;
    event->data.sequence_start.implicit = implicit;
    event->data.sequence_start.style = style;

    return event;
}

/*
 * Create a SEQUENCE-END event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_sequence_end_event_new(yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    return yaml_event_new(YAML_SEQUENCE_END_EVENT, start_mark, end_mark);
}

/*
 * Create a MAPPING-START event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_mapping_start_event_new(yaml_char_t *anchor, yaml_char_t *tag,
        int implicit, yaml_mapping_style_t style,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_event_t *event = yaml_event_new(YAML_MAPPING_START_EVENT,
            start_mark, end_mark);

    if (!event) return NULL;

    event->data.mapping_start.anchor = anchor;
    event->data.mapping_start.tag = tag;
    event->data.mapping_start.implicit = implicit;
    event->data.mapping_start.style = style;

    return event;
}

/*
 * Create a MAPPING-END event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_mapping_end_event_new(yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    return yaml_event_new(YAML_MAPPING_END_EVENT, start_mark, end_mark);
}

/*
 * Destroy an event object.
 */

YAML_DECLARE(void)
yaml_event_delete(yaml_event_t *event)
{
    assert(event);  /* Non-NULL event object expected. */

    switch (event->type)
    {
        case YAML_DOCUMENT_START_EVENT:
            /*yaml_free(event->data.document_start.version_directive);
            if (event->data.document_start.tag_directives) {
                yaml_tag_directive_t **tag_directive;
                for (tag_directive = event->data.document_start.tag_directives;
                        *tag_directive; tag_directive++) {
                    yaml_free((*tag_directive)->handle);
                    yaml_free((*tag_directive)->prefix);
                    yaml_free(*tag_directive);
                }
                yaml_free(event->data.document_start.tag_directives);
            }*/
            break;

        case YAML_ALIAS_EVENT:
            yaml_free(event->data.alias.anchor);
            break;

        case YAML_SCALAR_EVENT:
            yaml_free(event->data.scalar.anchor);
            yaml_free(event->data.scalar.tag);
            yaml_free(event->data.scalar.value);
            break;

        case YAML_SEQUENCE_START_EVENT:
            yaml_free(event->data.sequence_start.anchor);
            yaml_free(event->data.sequence_start.tag);
            break;

        case YAML_MAPPING_START_EVENT:
            yaml_free(event->data.mapping_start.anchor);
            yaml_free(event->data.mapping_start.tag);
            break;
    }

    memset(event, 0, sizeof(yaml_event_t));

    yaml_free(event);
}

