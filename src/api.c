
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml/yaml.h>

#include <assert.h>

/*
 * Allocate a dynamic memory block.
 */

void *
yaml_malloc(size_t size)
{
    return malloc(size ? size : 1);
}

/*
 * Reallocate a dynamic memory block.
 */

void *
yaml_realloc(void *ptr, size_t size)
{
    return ptr ? realloc(ptr, size ? size : 1) : malloc(size ? size : 1);
}

/*
 * Free a dynamic memory block.
 */

void
yaml_free(void *ptr)
{
    if (ptr) free(ptr);
}

/*
 * Create a new parser object.
 */

yaml_parser_t *
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

void
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

void
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

void
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

void
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

void
yaml_parser_set_encoding(yaml_parser_t *parser, yaml_encoding_t encoding)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->encoding); /* Encoding is already set or detected. */

    parser->encoding = encoding;
}

