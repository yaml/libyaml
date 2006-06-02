
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml/yaml.h>

#include <assert.h>

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
    if (!parser) return NULL;

    memset(parser, 0, sizeof(yaml_parser_t));

    /* Allocate the raw buffer. */

    parser->raw_buffer = yaml_malloc(YAML_RAW_BUFFER_SIZE);
    if (!parser->raw_buffer) {
        yaml_free(parser);
        return NULL;
    }
    parser->raw_pointer = parser->raw_buffer;
    parser->raw_unread = 0;

    /* Allocate the character buffer. */

    parser->buffer = yaml_malloc(YAML_BUFFER_SIZE);
    if (!parser->buffer) {
        yaml_free(parser->raw_buffer);
        yaml_free(parser);
        return NULL;
    }
    parser->buffer_end = parser->buffer;
    parser->pointer = parser->buffer;
    parser->unread = 0;

    return parser;
}

/*
 * Destroy a parser object.
 */

YAML_DECLARE(void)
yaml_parser_delete(yaml_parser_t *parser)
{
    assert(parser); /* Non-NULL parser object expected. */

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
yaml_stream_start_token(yaml_encoding_t encoding,
        yaml_mark_t start_mark, yaml_mark_t end_mark)
{
    yaml_token_t *token = yaml_token_new(YAML_STREAM_START_TOKEN,
            start_mark, end_mark);

    if (!token) return NULL;

    token->data.encoding = encoding;

    return token;
}

/*
 * Create a STREAM-END token.
 */

YAML_DECLARE(yaml_token_t *)
yaml_stream_end_token(yaml_mark_t start_mark, yaml_mark_t end_mark)
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

    token->data.anchor = anchor;

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

    token->data.anchor = anchor;

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
        case YAML_ANCHOR_TOKEN:
            yaml_free(token->data.anchor);
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

