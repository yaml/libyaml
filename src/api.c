
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

    parser = yaml_malloc(sizeof(yaml_parser_t));
    if (!parser) return NULL;

    memset(parser, 0, sizeof(yaml_parser_t));

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
    if (!parser->raw_buffer_foreign)
        yaml_free(parser->raw_buffer);

    memset(parser, 0, sizeof(yaml_parser_t));

    yaml_free(parser);
}

/*
 * String read handler (always returns error).
 */

static int
yaml_string_read_handler(void *data, unsigned char *buffer, size_t size,
        size_t *size_read)
{
    *size_read = 0;
    return 1;
}

/*
 * File read handler.
 */

static int
yaml_file_read_handler(void *data, unsigned char *buffer, size_t size,
        size_t *size_read)
{
    *size_read = fread(buffer, 1, size, (FILE *)ext);
    return !ferror((FILE *)ext);
}

/*
 * Set a string input.
 */

void
yaml_parser_set_input_string(yaml_parser_t *parser,
        unsigned char *input, size_t size)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->reader); /* You can set the source only once. */
    assert(input);  /* Non-NULL input string expected. */

    parser->read_handler = yaml_string_read_handler;
    parser->read_handler_data = NULL;

    /* We use the input string as a raw (undecoded) buffer. */
    parser->raw_buffer = input; 
    parser->raw_buffer_size = size;
    parser->raw_buffer_foreign = 1;
}

/*
 * Set a file input.
 */

void
yaml_parser_set_input_file(yaml_parser_t *parser, FILE *file)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->reader); /* You can set the source only once. */
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
    assert(!parser->reader); /* You can set the source only once. */
    assert(handler);    /* Non-NULL read handler expected. */

    parser->read_handler = handler;
    parser->read_handler_data = data
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

