
#include "yaml_private.h"

/*
 * Get the library version.
 */

YAML_DECLARE(const char *)
yaml_get_version_string(void)
{
    return YAML_VERSION_STRING;
}

/*
 * Get the library version numbers.
 */

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
 * Duplicate a string.
 */

YAML_DECLARE(yaml_char_t *)
yaml_strdup(const yaml_char_t *str)
{
    if (!str)
        return NULL;

    return (yaml_char_t *)strdup((char *)str);
}

/*
 * Extend a string.
 */

YAML_DECLARE(int)
yaml_string_extend(yaml_char_t **start,
        yaml_char_t **pointer, yaml_char_t **end)
{
    void *new_start = yaml_realloc(*start, (*end - *start)*2);

    if (!new_start) return 0;

    memset(new_start + (*end - *start), 0, *end - *start);

    *pointer = new_start + (*pointer - *start);
    *end = new_start + (*end - *start)*2;
    *start = new_start;

    return 1;
}

/*
 * Append a string B to a string A.
 */

YAML_DECLARE(int)
yaml_string_join(
        yaml_char_t **a_start, yaml_char_t **a_pointer, yaml_char_t **a_end,
        yaml_char_t **b_start, yaml_char_t **b_pointer, yaml_char_t **b_end)
{
    if (*b_start == *b_pointer)
        return 1;

    while (*a_end - *a_pointer <= *b_pointer - *b_start) {
        if (!yaml_string_extend(a_start, a_pointer, a_end))
            return 0;
    }

    memcpy(*a_pointer, *b_start, *b_pointer - *b_start);
    *a_pointer += *b_pointer - *b_start;

    return 1;
}

/*
 * Extend a stack.
 */

YAML_DECLARE(int)
yaml_stack_extend(void **start, void **top, void **end)
{
    void *new_start = yaml_realloc(*start, (*end - *start)*2);

    if (!new_start) return 0;

    *top = new_start + (*top - *start);
    *end = new_start + (*end - *start)*2;
    *start = new_start;

    return 1;
}

/*
 * Extend or move a queue.
 */

YAML_DECLARE(int)
yaml_queue_extend(void **start, void **head, void **tail, void **end)
{
    /* Check if we need to resize the queue. */

    if (*start == *head && *tail == *end) {
        void *new_start = yaml_realloc(*start, (*end - *start)*2);

        if (!new_start) return 0;

        *head = new_start + (*head - *start);
        *tail = new_start + (*tail - *start);
        *end = new_start + (*end - *start)*2;
        *start = new_start;
    }

    /* Check if we need to move the queue at the beginning of the buffer. */

    if (*tail == *end) {
        if (*head != *tail) {
            memmove(*start, *head, *tail - *head);
        }
        *tail -= *head - *start;
        *head = *start;
    }

    return 1;
}


/*
 * Create a new parser object.
 */

YAML_DECLARE(int)
yaml_parser_initialize(yaml_parser_t *parser)
{
    assert(parser);     /* Non-NULL parser object expected. */

    memset(parser, 0, sizeof(yaml_parser_t));
    if (!BUFFER_INIT(parser, parser->raw_buffer, INPUT_RAW_BUFFER_SIZE))
        goto error;
    if (!BUFFER_INIT(parser, parser->buffer, INPUT_BUFFER_SIZE))
        goto error;
    if (!QUEUE_INIT(parser, parser->tokens, INITIAL_QUEUE_SIZE))
        goto error;
    if (!STACK_INIT(parser, parser->indents, INITIAL_STACK_SIZE))
        goto error;
    if (!STACK_INIT(parser, parser->simple_keys, INITIAL_STACK_SIZE))
        goto error;
    if (!STACK_INIT(parser, parser->states, INITIAL_STACK_SIZE))
        goto error;
    if (!STACK_INIT(parser, parser->marks, INITIAL_STACK_SIZE))
        goto error;
    if (!STACK_INIT(parser, parser->tag_directives, INITIAL_STACK_SIZE))
        goto error;

    return 1;

error:

    BUFFER_DEL(parser, parser->raw_buffer);
    BUFFER_DEL(parser, parser->buffer);
    QUEUE_DEL(parser, parser->tokens);
    STACK_DEL(parser, parser->indents);
    STACK_DEL(parser, parser->simple_keys);
    STACK_DEL(parser, parser->states);
    STACK_DEL(parser, parser->marks);
    STACK_DEL(parser, parser->tag_directives);

    return 0;
}

/*
 * Destroy a parser object.
 */

YAML_DECLARE(void)
yaml_parser_delete(yaml_parser_t *parser)
{
    assert(parser); /* Non-NULL parser object expected. */

    BUFFER_DEL(parser, parser->raw_buffer);
    BUFFER_DEL(parser, parser->buffer);
    while (!QUEUE_EMPTY(parser, parser->tokens)) {
        yaml_token_delete(&DEQUEUE(parser, parser->tokens));
    }
    QUEUE_DEL(parser, parser->tokens);
    STACK_DEL(parser, parser->indents);
    STACK_DEL(parser, parser->simple_keys);
    STACK_DEL(parser, parser->states);
    STACK_DEL(parser, parser->marks);
    while (!STACK_EMPTY(parser, parser->tag_directives)) {
        yaml_tag_directive_t tag_directive = POP(parser, parser->tag_directives);
        yaml_free(tag_directive.handle);
        yaml_free(tag_directive.prefix);
    }
    STACK_DEL(parser, parser->tag_directives);

    memset(parser, 0, sizeof(yaml_parser_t));
}

/*
 * String read handler.
 */

static int
yaml_string_read_handler(void *data, unsigned char *buffer, size_t size,
        size_t *size_read)
{
    yaml_parser_t *parser = data;

    if (parser->input.string.current == parser->input.string.end) {
        *size_read = 0;
        return 1;
    }

    if (size > (parser->input.string.end - parser->input.string.current)) {
        size = parser->input.string.end - parser->input.string.current;
    }

    memcpy(buffer, parser->input.string.current, size);
    parser->input.string.current += size;
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
    yaml_parser_t *parser = data;

    *size_read = fread(buffer, 1, size, parser->input.file);
    return !ferror(parser->input.file);
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

    parser->read_handler = yaml_string_read_handler;
    parser->read_handler_data = parser;

    parser->input.string.start = input;
    parser->input.string.current = input;
    parser->input.string.end = input+size;
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
    parser->read_handler_data = parser;

    parser->input.file = file;
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
 * Create a new emitter object.
 */

YAML_DECLARE(int)
yaml_emitter_initialize(yaml_emitter_t *emitter)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    memset(emitter, 0, sizeof(yaml_emitter_t));
    if (!BUFFER_INIT(emitter, emitter->buffer, OUTPUT_BUFFER_SIZE))
        goto error;
    if (!BUFFER_INIT(emitter, emitter->raw_buffer, OUTPUT_RAW_BUFFER_SIZE))
        goto error;
    if (!STACK_INIT(emitter, emitter->states, INITIAL_STACK_SIZE))
        goto error;
    if (!QUEUE_INIT(emitter, emitter->events, INITIAL_QUEUE_SIZE))
        goto error;
    if (!STACK_INIT(emitter, emitter->indents, INITIAL_STACK_SIZE))
        goto error;
    if (!STACK_INIT(emitter, emitter->tag_directives, INITIAL_STACK_SIZE))
        goto error;

    return 1;

error:

    BUFFER_DEL(emitter, emitter->buffer);
    BUFFER_DEL(emitter, emitter->raw_buffer);
    STACK_DEL(emitter, emitter->states);
    QUEUE_DEL(emitter, emitter->events);
    STACK_DEL(emitter, emitter->indents);
    STACK_DEL(emitter, emitter->tag_directives);

    return 0;
}

/*
 * Destroy an emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_delete(yaml_emitter_t *emitter)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    BUFFER_DEL(emitter, emitter->buffer);
    BUFFER_DEL(emitter, emitter->raw_buffer);
    STACK_DEL(emitter, emitter->states);
    while (!QUEUE_EMPTY(emitter, emitter->events)) {
        yaml_event_delete(&DEQUEUE(emitter, emitter->events));
    }
    STACK_DEL(emitter, emitter->indents);
    yaml_event_delete(&emitter->event);
    while (!STACK_EMPTY(empty, emitter->tag_directives)) {
        yaml_tag_directive_t tag_directive = POP(emitter, emitter->tag_directives);
        yaml_free(tag_directive.handle);
        yaml_free(tag_directive.prefix);
    }
    STACK_DEL(emitter, emitter->tag_directives);

    memset(emitter, 0, sizeof(yaml_emitter_t));
}

/*
 * String write handler.
 */

static int
yaml_string_write_handler(void *data, unsigned char *buffer, size_t size)
{
    yaml_emitter_t *emitter = data;

    if (emitter->output.string.size + *emitter->output.string.size_written
            < size) {
        memcpy(emitter->output.string.buffer
                + *emitter->output.string.size_written,
                buffer,
                emitter->output.string.size
                - *emitter->output.string.size_written);
        *emitter->output.string.size_written = emitter->output.string.size;
        return 0;
    }

    memcpy(emitter->output.string.buffer
            + *emitter->output.string.size_written, buffer, size);
    *emitter->output.string.size_written += size;
    return 1;
}

/*
 * File write handler.
 */

static int
yaml_file_write_handler(void *data, unsigned char *buffer, size_t size)
{
    yaml_emitter_t *emitter = data;

    return (fwrite(buffer, 1, size, emitter->output.file) == size);
}
/*
 * Set a string output.
 */

YAML_DECLARE(void)
yaml_emitter_set_output_string(yaml_emitter_t *emitter,
        unsigned char *output, size_t size, size_t *size_written)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->write_handler);    /* You can set the output only once. */
    assert(output);     /* Non-NULL output string expected. */

    emitter->write_handler = yaml_string_write_handler;
    emitter->write_handler_data = emitter;

    emitter->output.string.buffer = output;
    emitter->output.string.size = size;
    emitter->output.string.size_written = size_written;
    *size_written = 0;
}

/*
 * Set a file output.
 */

YAML_DECLARE(void)
yaml_emitter_set_output_file(yaml_emitter_t *emitter, FILE *file)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->write_handler);    /* You can set the output only once. */
    assert(file);       /* Non-NULL file object expected. */

    emitter->write_handler = yaml_file_write_handler;
    emitter->write_handler_data = emitter;

    emitter->output.file = file;
}

/*
 * Set a generic output handler.
 */

YAML_DECLARE(void)
yaml_emitter_set_output(yaml_emitter_t *emitter,
        yaml_write_handler_t *handler, void *data)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->write_handler);    /* You can set the output only once. */
    assert(handler);    /* Non-NULL handler object expected. */

    emitter->write_handler = handler;
    emitter->write_handler_data = data;
}

/*
 * Set the output encoding.
 */

YAML_DECLARE(void)
yaml_emitter_set_encoding(yaml_emitter_t *emitter, yaml_encoding_t encoding)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->encoding);     /* You can set encoding only once. */

    emitter->encoding = encoding;
}

/*
 * Set the canonical output style.
 */

YAML_DECLARE(void)
yaml_emitter_set_canonical(yaml_emitter_t *emitter, int canonical)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    emitter->canonical = (canonical != 0);
}

/*
 * Set the indentation increment.
 */

YAML_DECLARE(void)
yaml_emitter_set_indent(yaml_emitter_t *emitter, int indent)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    emitter->best_indent = (1 < indent && indent < 10) ? indent : 2;
}

/*
 * Set the preferred line width.
 */

YAML_DECLARE(void)
yaml_emitter_set_width(yaml_emitter_t *emitter, int width)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    emitter->best_width = (width >= 0) ? width : -1;
}

/*
 * Set if unescaped non-ASCII characters are allowed.
 */

YAML_DECLARE(void)
yaml_emitter_set_unicode(yaml_emitter_t *emitter, int unicode)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    emitter->unicode = (unicode != 0);
}

/*
 * Set the preferred line break character.
 */

YAML_DECLARE(void)
yaml_emitter_set_break(yaml_emitter_t *emitter, yaml_break_t line_break)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    emitter->line_break = line_break;
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

        default:
            break;
    }

    memset(token, 0, sizeof(yaml_token_t));
}

/*
 * Destroy an event object.
 */

YAML_DECLARE(void)
yaml_event_delete(yaml_event_t *event)
{
    yaml_tag_directive_t *tag_directive;

    assert(event);  /* Non-NULL event object expected. */

    switch (event->type)
    {
        case YAML_DOCUMENT_START_EVENT:
            yaml_free(event->data.document_start.version_directive);
            for (tag_directive = event->data.document_start.tag_directives.start;
                    tag_directive != event->data.document_start.tag_directives.end;
                    tag_directive++) {
                yaml_free(tag_directive->handle);
                yaml_free(tag_directive->prefix);
            }
            yaml_free(event->data.document_start.tag_directives.start);
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

        default:
            break;
    }

    memset(event, 0, sizeof(yaml_event_t));
}

