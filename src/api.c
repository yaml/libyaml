
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
 * Format an error message.
 */

YAML_DECLARE(int)
yaml_error_message(yaml_error_t *error, char *buffer, size_t capacity)
{
    char *prefixes[] = {
        "No error",
        "Memory error",
        "Reader error",
        "Decoder error",
        "Scanner error",
        "Parser error",
        "Composer error",
        "Writer error",
        "Emitter error",
        "Serializer error",
        "Resolver error",
    };
    int length;

    assert(error);  /* Non-NULL error is expected. */
    assert(buffer); /* Non-NULL buffer is expected. */

    switch (error->type)
    {
        case YAML_NO_ERROR:
        case YAML_MEMORY_ERROR:
            length = snprintf(buffer, capacity, "%s",
                    prefixes[error->type]);
            break;

        case YAML_READER_ERROR:
        case YAML_DECODER_ERROR:
            if (error->data.reading.value == -1) {
                length = snprintf(buffer, capacity,
                        "%s: %s at position %d",
                        prefixes[error->type],
                        error->data.reading.problem,
                        error->data.reading.offset);
            }
            else {
                length = snprintf(buffer, capacity,
                        "%s: %s (#%X) at position %d",
                        prefixes[error->type],
                        error->data.reading.problem,
                        error->data.reading.value,
                        error->data.reading.offset);
            }
            break;

        case YAML_SCANNER_ERROR:
        case YAML_PARSER_ERROR:
        case YAML_COMPOSER_ERROR:
            if (!error->data.loading.context) {
                length = snprintf(buffer, capacity,
                        "%s: %s at line %d, column %d",
                        prefixes[error->type],
                        error->data.loading.problem,
                        error->data.loading.problem_mark.line+1,
                        error->data.loading.problem_mark.column+1);
            }
            else {
                length = snprintf(buffer, capacity,
                        "%s: %s at line %d, column %d, %s at line %d, column %d",
                        prefixes[error->type],
                        error->data.loading.context,
                        error->data.loading.context_mark.line+1,
                        error->data.loading.context_mark.column+1,
                        error->data.loading.problem,
                        error->data.loading.problem_mark.line+1,
                        error->data.loading.problem_mark.column+1);
            }
            break;

        case YAML_WRITER_ERROR:
            length = snprintf(buffer, capacity,
                    "%s: %s at position %d",
                    prefixes[error->type],
                    error->data.writing.problem,
                    error->data.writing.offset);
            break;

        case YAML_EMITTER_ERROR:
        case YAML_SERIALIZER_ERROR:
            length = snprintf(buffer, capacity, "%s: %s",
                    prefixes[error->type],
                    error->data.dumping.problem);
            break;

        case YAML_RESOLVER_ERROR:
            length = snprintf(buffer, capacity, "%s: %s",
                    prefixes[error->type],
                    error->data.resolving.problem);
            break;

        default:
            assert(0);  /* Should never happen. */
    }

    return (length >= 0 && length < capacity);
}


/*
 * Extend a string.
 */

YAML_DECLARE(int)
yaml_ostring_extend(yaml_char_t **buffer, size_t *capacity)
{
    yaml_char_t *new_buffer = yaml_realloc(*buffer, (*capacity)*2);

    if (!new_buffer) return 0;

    memset(new_buffer + *capacity, 0, *capacity);

    *buffer = new_buffer;
    *capacity *= 2;

    return 1;
}

/*
 * Append an adjunct string to a base string.
 */

YAML_DECLARE(int)
yaml_ostring_join(
        yaml_char_t **base_buffer, size_t *base_pointer, size_t *base_capacity,
        yaml_char_t *adj_buffer, size_t adj_pointer)
{
    if (!adj_pointer)
        return 1;

    while (*base_capacity - *base_pointer <= adj_pointer) {
        if (!yaml_ostring_extend(base_buffer, base_capacity))
            return 0;
    }

    memcpy(*base_buffer+*base_pointer, adj_buffer, adj_pointer);
    *base_pointer += adj_pointer;

    return 1;
}

/*
 * Extend a stack.
 */

YAML_DECLARE(int)
yaml_stack_extend(void **list, size_t size, size_t *length, size_t *capacity)
{
    void *new_list = yaml_realloc(*list, (*capacity)*size*2);

    if (!new_list) return 0;

    *list = new_list;
    *capacity *= 2;

    return 1;
}

/*
 * Extend or move a queue.
 */

YAML_DECLARE(int)
yaml_queue_extend(void **list, size_t size,
        size_t *head, size_t *tail, size_t *capacity)
{
    /* Check if we need to resize the queue. */

    if (*head == 0 && *tail == *capacity) {
        void *new_list = yaml_realloc(*list, (*capacity)*size*2);

        if (!new_list) return 0;

        *list = new_list;
        *capacity *= 2;
    }

    /* Check if we need to move the queue at the beginning of the buffer. */

    if (*tail == *capacity) {
        if (*head != *tail) {
            memmove((char *)*list, (char *)*list + (*head)*size,
                    (*tail-*head)*size);
        }
        *tail -= *head;
        *head = 0;
    }

    return 1;
}


/*
 * Create a new parser object.
 */

YAML_DECLARE(yaml_parser_t *)
yaml_parser_new(void)
{
    yaml_parser_t *parser = yaml_malloc(sizeof(yaml_parser_t));

    if (!parser)
        return NULL;

    memset(parser, 0, sizeof(yaml_parser_t));
    if (!IOSTRING_INIT(parser, parser->raw_input, RAW_INPUT_BUFFER_CAPACITY))
        goto error;
    if (!IOSTRING_INIT(parser, parser->input, INPUT_BUFFER_CAPACITY))
        goto error;
    if (!QUEUE_INIT(parser, parser->tokens, INITIAL_QUEUE_CAPACITY))
        goto error;
    if (!STACK_INIT(parser, parser->indents, INITIAL_STACK_CAPACITY))
        goto error;
    if (!STACK_INIT(parser, parser->simple_keys, INITIAL_STACK_CAPACITY))
        goto error;
    if (!STACK_INIT(parser, parser->states, INITIAL_STACK_CAPACITY))
        goto error;
    if (!STACK_INIT(parser, parser->marks, INITIAL_STACK_CAPACITY))
        goto error;
    if (!STACK_INIT(parser, parser->tag_directives, INITIAL_STACK_CAPACITY))
        goto error;

    return parser;

error:
    yaml_parser_delete(parser);

    return NULL;
}

/*
 * Destroy a parser object.
 */

YAML_DECLARE(void)
yaml_parser_delete(yaml_parser_t *parser)
{
    assert(parser); /* Non-NULL parser object expected. */

    IOSTRING_DEL(parser, parser->raw_input);
    IOSTRING_DEL(parser, parser->input);
    while (!QUEUE_EMPTY(parser, parser->tokens)) {
        yaml_token_destroy(&DEQUEUE(parser, parser->tokens));
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
    yaml_free(parser);
}

/*
 * Get the current parser error.
 */

YAML_DECLARE(void)
yaml_parser_get_error(yaml_parser_t *parser, yaml_error_t *error)
{
    assert(parser); /* Non-NULL parser object expected. */

    *error = parser->error;
}

/*
 * Standard string read handler.
 */

static int
yaml_string_reader(void *untyped_data, unsigned char *buffer, size_t capacity,
        size_t *length)
{
    yaml_standard_reader_data_t *data = untyped_data;

    if (data->string.pointer == data->string.length) {
        *length = 0;
        return 1;
    }

    if (capacity > (size_t)(data->string.length - data->string.pointer)) {
        capacity = data->string.length - data->string.pointer;
    }

    memcpy(buffer, data->string.buffer + data->string.pointer, capacity);
    data->string.pointer += capacity;
    *length = capacity;
    return 1;
}

/*
 * Standard file read handler.
 */

static int
yaml_file_reader(void *untyped_data, unsigned char *buffer, size_t capacity,
        size_t *length)
{
    yaml_standard_reader_data_t *data = untyped_data;

    *length = fread(buffer, 1, capacity, data->file);
    return !ferror(data->file);
}

/*
 * Set a string input.
 */

YAML_DECLARE(void)
yaml_parser_set_string_reader(yaml_parser_t *parser,
        const unsigned char *buffer, size_t length)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->reader);    /* You can set the input handler only once. */
    assert(buffer); /* Non-NULL input string expected. */

    parser->reader = yaml_string_reader;
    parser->reader_data = &(parser->standard_reader_data);

    parser->standard_reader_data.string.buffer = buffer;
    parser->standard_reader_data.string.pointer = 0;
    parser->standard_reader_data.string.length = length;
}

/*
 * Set a file input.
 */

YAML_DECLARE(void)
yaml_parser_set_file_reader(yaml_parser_t *parser, FILE *file)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->reader);    /* You can set the input handler only once. */
    assert(file);   /* Non-NULL file object expected. */

    parser->reader = yaml_file_reader;
    parser->reader_data = &(parser->standard_reader_data);

    parser->standard_reader_data.file = file;
}

/*
 * Set a generic input.
 */

YAML_DECLARE(void)
yaml_parser_set_reader(yaml_parser_t *parser,
        yaml_reader_t *reader, void *data)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->reader);    /* You can set the input handler only once. */
    assert(reader); /* Non-NULL read handler expected. */

    parser->reader = reader;
    parser->reader_data = data;
}

/*
 * Set the source encoding.
 */

YAML_DECLARE(void)
yaml_parser_set_encoding(yaml_parser_t *parser, yaml_encoding_t encoding)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->encoding);  /* Encoding is already set or detected. */

    parser->encoding = encoding;
}

/*
 * Create a new emitter object.
 */

YAML_DECLARE(yaml_emitter_t *)
yaml_emitter_new(void)
{
    yaml_emitter_t *emitter = yaml_malloc(sizeof(yaml_emitter_t));

    if (!emitter)
        return NULL;

    memset(emitter, 0, sizeof(yaml_emitter_t));
    if (!IOSTRING_INIT(emitter, emitter->output, OUTPUT_BUFFER_CAPACITY))
        goto error;
    if (!IOSTRING_INIT(emitter, emitter->raw_output, RAW_OUTPUT_BUFFER_CAPACITY))
        goto error;
    if (!STACK_INIT(emitter, emitter->states, INITIAL_STACK_CAPACITY))
        goto error;
    if (!QUEUE_INIT(emitter, emitter->events, INITIAL_QUEUE_CAPACITY))
        goto error;
    if (!STACK_INIT(emitter, emitter->indents, INITIAL_STACK_CAPACITY))
        goto error;
    if (!STACK_INIT(emitter, emitter->tag_directives, INITIAL_STACK_CAPACITY))
        goto error;

    return emitter;

error:
    yaml_emitter_delete(emitter);

    return NULL;
}

/*
 * Destroy an emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_delete(yaml_emitter_t *emitter)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    IOSTRING_DEL(emitter, emitter->output);
    IOSTRING_DEL(emitter, emitter->raw_output);
    STACK_DEL(emitter, emitter->states);
    while (!QUEUE_EMPTY(emitter, emitter->events)) {
        yaml_event_delete(&DEQUEUE(emitter, emitter->events));
    }
    QUEUE_DEL(emitter, emitter->events);
    STACK_DEL(emitter, emitter->indents);
    while (!STACK_EMPTY(empty, emitter->tag_directives)) {
        yaml_tag_directive_t tag_directive = POP(emitter, emitter->tag_directives);
        yaml_free(tag_directive.handle);
        yaml_free(tag_directive.prefix);
    }
    STACK_DEL(emitter, emitter->tag_directives);
    yaml_free(emitter->anchors);

    memset(emitter, 0, sizeof(yaml_emitter_t));
    yaml_free(emitter);
}

/*
 * Get the current emitter error.
 */

YAML_DECLARE(void)
yaml_emitter_get_error(yaml_emitter_t *emitter, yaml_error_t *error)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    *error = emitter->error;
}

/*
 * String write handler.
 */

static int
yaml_string_writer(void *untyped_data, const unsigned char *buffer, size_t length)
{
    yaml_standard_writer_data_t *data = untyped_data;
    int result = 1;

    if (data->string.capacity - data->string.pointer < length) {
        length = data->string.capacity - data->string.pointer;
        result = 0;
    }

    memcpy(data->string.buffer + data->string.pointer, buffer, length);
    data->string.pointer += length;
    *data->length += length;

    return result;
}

/*
 * File write handler.
 */

static int
yaml_file_writer(void *untyped_data, const unsigned char *buffer, size_t length)
{
    yaml_standard_writer_data_t *data = untyped_data;

    return (fwrite(buffer, 1, length, data->file) == length);
}
/*
 * Set a string output.
 */

YAML_DECLARE(void)
yaml_emitter_set_string_writer(yaml_emitter_t *emitter,
        unsigned char *buffer, size_t *length, size_t capacity)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->writer);   /* You can set the output only once. */
    assert(buffer);     /* Non-NULL output string expected. */

    emitter->writer = yaml_string_writer;
    emitter->writer_data = &(emitter->standard_writer_data);

    emitter->standard_writer_data.string.buffer = buffer;
    emitter->standard_writer_data.string.pointer = 0;
    emitter->standard_writer_data.string.capacity = capacity;
    emitter->standard_writer_data.length = length;

    *length = 0;
}

/*
 * Set a file output.
 */

YAML_DECLARE(void)
yaml_emitter_set_file_writer(yaml_emitter_t *emitter, FILE *file)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->writer);   /* You can set the output only once. */
    assert(file);       /* Non-NULL file object expected. */

    emitter->writer = yaml_string_writer;
    emitter->writer_data = &(emitter->standard_writer_data);

    emitter->standard_writer_data.file = file;
}

/*
 * Set a generic output handler.
 */

YAML_DECLARE(void)
yaml_emitter_set_writer(yaml_emitter_t *emitter,
        yaml_writer_t *writer, void *data)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->writer);   /* You can set the output only once. */
    assert(writer); /* Non-NULL handler object expected. */

    emitter->writer = writer;
    emitter->writer_data = data;
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
yaml_emitter_set_canonical(yaml_emitter_t *emitter, int is_canonical)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    emitter->is_canonical = (is_canonical != 0);
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
yaml_emitter_set_unicode(yaml_emitter_t *emitter, int is_unicode)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    emitter->is_unicode = (is_unicode != 0);
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
 * Allocate a token object.
 */

YAML_DECLARE(yaml_token_t *)
yaml_token_new(void)
{
    yaml_token_t *token = yaml_malloc(sizeof(yaml_token_t));

    if (!token)
        return NULL;

    memset(token, 0, sizeof(yaml_token_t));

    return token;
}

/*
 * Deallocate a token object.
 */

YAML_DECLARE(void)
yaml_token_delete(yaml_token_t *token)
{
    assert(token);  /* Non-NULL token object expected. */

    yaml_token_destroy(token);
    yaml_free(token);
}

/*
 * Duplicate a token object.
 */

YAML_DECLARE(int)
yaml_token_duplicate(yaml_token_t *token, const yaml_token_t *model)
{
    assert(token);  /* Non-NULL token object is expected. */
    assert(model);  /* Non-NULL model token object is expected. */

    memset(token, 0, sizeof(yaml_token_t));

    token->type = model->type;
    token->start_mark = model->start_mark;
    token->end_mark = model->end_mark;

    switch (token->type)
    {
        case YAML_STREAM_START_TOKEN:
            token->data.stream_start.encoding =
                model->data.stream_start.encoding;
            break;

        case YAML_VERSION_DIRECTIVE_TOKEN:
            token->data.version_directive.major =
                model->data.version_directive.major;
            token->data.version_directive.minor =
                model->data.version_directive.minor;
            break;

        case YAML_TAG_DIRECTIVE_TOKEN:
            if (!(token->data.tag_directive.handle =
                        yaml_strdup(model->data.tag_directive.handle)))
                goto error;
            if (!(token->data.tag_directive.prefix =
                        yaml_strdup(model->data.tag_directive.prefix)))
                goto error;
            break;

        case YAML_ALIAS_TOKEN:
            if (!(token->data.alias.value =
                        yaml_strdup(model->data.alias.value)))
                goto error;
            break;

        case YAML_ANCHOR_TOKEN:
            if (!(token->data.anchor.value =
                        yaml_strdup(model->data.anchor.value)))
                goto error;
            break;

        case YAML_TAG_TOKEN:
            if (!(token->data.tag.handle = yaml_strdup(model->data.tag.handle)))
                goto error;
            if (!(token->data.tag.suffix = yaml_strdup(model->data.tag.suffix)))
                goto error;
            break;

        case YAML_SCALAR_TOKEN:
            if (!(token->data.scalar.value =
                        yaml_malloc(model->data.scalar.length+1)))
                goto error;
            memcpy(token->data.scalar.value, model->data.scalar.value,
                    model->data.scalar.length+1);
            token->data.scalar.length = model->data.scalar.length;
            break;

        default:
            break;
    }

    return 1;

error:
    yaml_token_destroy(token);

    return 0;
}

/*
 * Destroy a token object.
 */

YAML_DECLARE(void)
yaml_token_destroy(yaml_token_t *token)
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
 * Check if a string is a valid UTF-8 sequence.
 *
 * Check 'reader.c' for more details on UTF-8 encoding.
 */

static int
yaml_valid_utf8(const yaml_char_t *buffer, size_t length)
{
    size_t pointer = 0;

    while (pointer < length) {
        unsigned char octet;
        unsigned int width;
        unsigned int value;
        size_t k;

        octet = buffer[pointer];
        width = (octet & 0x80) == 0x00 ? 1 :
                (octet & 0xE0) == 0xC0 ? 2 :
                (octet & 0xF0) == 0xE0 ? 3 :
                (octet & 0xF8) == 0xF0 ? 4 : 0;
        value = (octet & 0x80) == 0x00 ? octet & 0x7F :
                (octet & 0xE0) == 0xC0 ? octet & 0x1F :
                (octet & 0xF0) == 0xE0 ? octet & 0x0F :
                (octet & 0xF8) == 0xF0 ? octet & 0x07 : 0;
        if (!width) return 0;
        if (pointer+width > length) return 0;
        for (k = 1; k < width; k ++) {
            octet = buffer[pointer+k];
            if ((octet & 0xC0) != 0x80) return 0;
            value = (value << 6) + (octet & 0x3F);
        }
        if (!((width == 1) ||
            (width == 2 && value >= 0x80) ||
            (width == 3 && value >= 0x800) ||
            (width == 4 && value >= 0x10000))) return 0;

        pointer += width;
    }

    return 1;
}

/*
 * Allocate an event object.
 */

YAML_DECLARE(yaml_event_t *)
yaml_event_new(void)
{
    yaml_event_t *event = yaml_malloc(sizeof(yaml_event_t));

    if (!event)
        return NULL;

    memset(event, 0, sizeof(yaml_event_t));

    return event;
}

/*
 * Deallocate an event object.
 */

YAML_DECLARE(void)
yaml_event_delete(yaml_event_t *event)
{
    assert(event);  /* Non-NULL event object expected. */

    yaml_event_destroy(event);
    yaml_free(event);
}

/*
 * Duplicate an event object.
 */

YAML_DECLARE(int)
yaml_event_duplicate(yaml_event_t *event, const yaml_event_t *model)
{
    struct {
        yaml_error_t error;
    } self;

    assert(event);  /* Non-NULL event object is expected. */
    assert(model);  /* Non-NULL model event object is expected. */

    memset(event, 0, sizeof(yaml_event_t));

    event->type = model->type;
    event->start_mark = model->start_mark;
    event->end_mark = model->end_mark;

    switch (event->type)
    {
        case YAML_STREAM_START_EVENT:
            event->data.stream_start.encoding =
                model->data.stream_start.encoding;
            break;

        case YAML_DOCUMENT_START_EVENT:
            if (model->data.document_start.version_directive) {
                if (!(event->data.document_start.version_directive =
                            yaml_malloc(sizeof(yaml_version_directive_t))))
                    goto error;
                *event->data.document_start.version_directive =
                    *model->data.document_start.version_directive;
            }
            if (model->data.document_start.tag_directives.length) {
                yaml_tag_directive_t *tag_directives =
                    model->data.document_start.tag_directives.list;
                if (!STACK_INIT(&self, event->data.document_start.tag_directives,
                            model->data.document_start.tag_directives.capacity))
                    goto error;
                while (event->data.document_start.tag_directives.length !=
                        model->data.document_start.tag_directives.length) {
                    yaml_tag_directive_t value;
                    value.handle = yaml_strdup(tag_directives->handle);
                    value.prefix = yaml_strdup(tag_directives->prefix);
                    PUSH(&self, event->data.document_start.tag_directives, value);
                    if (!value.handle || !value.prefix)
                        goto error;
                    tag_directives ++;
                }
            }
            event->data.document_start.is_implicit =
                model->data.document_start.is_implicit;
            break;

        case YAML_DOCUMENT_END_EVENT:
            event->data.document_end.is_implicit =
                model->data.document_end.is_implicit;

        case YAML_ALIAS_EVENT:
            if (!(event->data.alias.anchor =
                        yaml_strdup(model->data.alias.anchor)))
                goto error;
            break;

        case YAML_SCALAR_EVENT:
            if (model->data.scalar.anchor &&
                    !(event->data.scalar.anchor =
                        yaml_strdup(model->data.scalar.anchor)))
                goto error;
            if (event->data.scalar.tag &&
                    !(event->data.scalar.tag =
                        yaml_strdup(model->data.scalar.tag)))
                goto error;
            if (!(event->data.scalar.value =
                        yaml_malloc(model->data.scalar.length+1)))
                goto error;
            memcpy(event->data.scalar.value, model->data.scalar.value,
                    model->data.scalar.length+1);
            event->data.scalar.length = model->data.scalar.length;
            event->data.scalar.is_plain_implicit =
                model->data.scalar.is_plain_implicit;
            event->data.scalar.is_quoted_implicit =
                model->data.scalar.is_quoted_implicit;
            event->data.scalar.style = model->data.scalar.style;
            break;

        case YAML_SEQUENCE_START_EVENT:
            if (model->data.sequence_start.anchor &&
                    !(event->data.sequence_start.anchor =
                        yaml_strdup(model->data.sequence_start.anchor)))
                goto error;
            if (event->data.sequence_start.tag &&
                    !(event->data.sequence_start.tag =
                        yaml_strdup(model->data.sequence_start.tag)))
                goto error;
            event->data.sequence_start.is_implicit =
                model->data.sequence_start.is_implicit;
            event->data.sequence_start.style =
                model->data.sequence_start.style;
            break;

        case YAML_MAPPING_START_EVENT:
            if (model->data.mapping_start.anchor &&
                    !(event->data.mapping_start.anchor =
                        yaml_strdup(model->data.mapping_start.anchor)))
                goto error;
            if (event->data.mapping_start.tag &&
                    !(event->data.mapping_start.tag =
                        yaml_strdup(model->data.mapping_start.tag)))
                goto error;
            event->data.mapping_start.is_implicit =
                model->data.mapping_start.is_implicit;
            event->data.mapping_start.style =
                model->data.mapping_start.style;
            break;

        default:
            break;
    }

    return 1;

error:
    yaml_event_destroy(event);

    return 0;
}

/*
 * Create STREAM-START.
 */

YAML_DECLARE(int)
yaml_event_create_stream_start(yaml_event_t *event,
        yaml_encoding_t encoding)
{
    yaml_mark_t mark = { 0, 0, 0 };

    assert(event);  /* Non-NULL event object is expected. */

    STREAM_START_EVENT_INIT(*event, encoding, mark, mark);

    return 1;
}

/*
 * Create STREAM-END.
 */

YAML_DECLARE(int)
yaml_event_create_stream_end(yaml_event_t *event)
{
    yaml_mark_t mark = { 0, 0, 0 };

    assert(event);  /* Non-NULL event object is expected. */

    STREAM_END_EVENT_INIT(*event, mark, mark);

    return 1;
}

/*
 * Create DOCUMENT-START.
 */

YAML_DECLARE(int)
yaml_event_create_document_start(yaml_event_t *event,
        const yaml_version_directive_t *version_directive,
        const yaml_tag_directive_t *tag_directives,
        int is_implicit)
{
    struct {
        yaml_error_t error;
    } self;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_version_directive_t *version_directive_copy = NULL;
    struct {
        yaml_tag_directive_t *list;
        size_t length;
        size_t capacity;
    } tag_directives_copy = { NULL, 0, 0 };

    assert(event);          /* Non-NULL event object is expected. */

    if (version_directive) {
        version_directive_copy = yaml_malloc(sizeof(yaml_version_directive_t));
        if (!version_directive_copy) goto error;
        *version_directive_copy = *version_directive;
    }

    if (tag_directives && (tag_directives->handle || tag_directives->prefix)) {
        if (!STACK_INIT(&self, tag_directives_copy, INITIAL_STACK_CAPACITY))
            goto error;
        while (tag_directives->handle || tag_directives->prefix) {
            yaml_tag_directive_t value = *tag_directives;
            assert(value.handle);
            assert(value.prefix);
            if (!yaml_valid_utf8(value.handle, strlen((char *)value.handle)))
                goto error;
            if (!yaml_valid_utf8(value.prefix, strlen((char *)value.prefix)))
                goto error;
            if (!PUSH(&self, tag_directives_copy, value))
                goto error;
            value.handle = yaml_strdup(value.handle);
            value.prefix = yaml_strdup(value.prefix);
            tag_directives_copy.list[tag_directives_copy.length-1] = value;
            if (!value.handle || !value.prefix)
                goto error;
        }
    }

    DOCUMENT_START_EVENT_INIT(*event, version_directive_copy,
            tag_directives_copy.list, tag_directives_copy.length,
            tag_directives_copy.capacity, is_implicit, mark, mark);

    return 1;

error:
    yaml_free(version_directive_copy);
    while (!STACK_EMPTY(&self, tag_directives_copy)) {
        yaml_tag_directive_t value = POP(&self, tag_directives_copy);
        yaml_free(value.handle);
        yaml_free(value.prefix);
    }
    STACK_DEL(&self, tag_directives_copy);

    return 0;
}

/*
 * Create DOCUMENT-END.
 */

YAML_DECLARE(int)
yaml_event_create_document_end(yaml_event_t *event, int is_implicit)
{
    yaml_mark_t mark = { 0, 0, 0 };

    assert(event);      /* Non-NULL emitter object is expected. */

    DOCUMENT_END_EVENT_INIT(*event, is_implicit, mark, mark);

    return 1;
}

/*
 * Create ALIAS.
 */

YAML_DECLARE(int)
yaml_event_create_alias(yaml_event_t *event, const yaml_char_t *anchor)
{
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;

    assert(event);      /* Non-NULL event object is expected. */
    assert(anchor);     /* Non-NULL anchor is expected. */

    if (!yaml_valid_utf8(anchor, strlen((char *)anchor))) return 0;

    anchor_copy = yaml_strdup(anchor);
    if (!anchor_copy)
        return 0;

    ALIAS_EVENT_INIT(*event, anchor_copy, mark, mark);

    return 1;
}

/*
 * Create SCALAR.
 */

YAML_DECLARE(int)
yaml_event_create_scalar(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        const yaml_char_t *value, size_t length,
        int is_plain_implicit, int is_quoted_implicit,
        yaml_scalar_style_t style)
{
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;
    yaml_char_t *value_copy = NULL;

    assert(event);      /* Non-NULL event object is expected. */
    assert(value);      /* Non-NULL anchor is expected. */

    if (anchor) {
        if (!yaml_valid_utf8(anchor, strlen((char *)anchor)))
            goto error;
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy)
            goto error;
    }

    if (tag) {
        if (!yaml_valid_utf8(tag, strlen((char *)tag)))
            goto error;
        tag_copy = yaml_strdup(tag);
        if (!tag_copy)
            goto error;
    }

    if (length < 0) {
        length = strlen((char *)value);
    }

    if (!yaml_valid_utf8(value, length))
        goto error;
    value_copy = yaml_malloc(length+1);
    if (!value_copy)
        goto error;
    memcpy(value_copy, value, length);
    value_copy[length] = '\0';

    SCALAR_EVENT_INIT(*event, anchor_copy, tag_copy, value_copy, length,
            is_plain_implicit, is_quoted_implicit, style, mark, mark);

    return 1;

error:
    yaml_free(anchor_copy);
    yaml_free(tag_copy);
    yaml_free(value_copy);

    return 0;
}

/*
 * Create SEQUENCE-START.
 */

YAML_DECLARE(int)
yaml_event_create_sequence_start(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        int is_implicit, yaml_sequence_style_t style)
{
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;

    assert(event);      /* Non-NULL event object is expected. */

    if (anchor) {
        if (!yaml_valid_utf8(anchor, strlen((char *)anchor)))
            goto error;
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy)
            goto error;
    }

    if (tag) {
        if (!yaml_valid_utf8(tag, strlen((char *)tag)))
            goto error;
        tag_copy = yaml_strdup(tag);
        if (!tag_copy)
            goto error;
    }

    SEQUENCE_START_EVENT_INIT(*event, anchor_copy, tag_copy,
            is_implicit, style, mark, mark);

    return 1;

error:
    yaml_free(anchor_copy);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Create SEQUENCE-END.
 */

YAML_DECLARE(int)
yaml_event_create_sequence_end(yaml_event_t *event)
{
    yaml_mark_t mark = { 0, 0, 0 };

    assert(event);      /* Non-NULL event object is expected. */

    SEQUENCE_END_EVENT_INIT(*event, mark, mark);

    return 1;
}

/*
 * Create MAPPING-START.
 */

YAML_DECLARE(int)
yaml_event_create_mapping_start(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        int is_implicit, yaml_mapping_style_t style)
{
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;

    assert(event);      /* Non-NULL event object is expected. */

    if (anchor) {
        if (!yaml_valid_utf8(anchor, strlen((char *)anchor)))
            goto error;
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy)
            goto error;
    }

    if (tag) {
        if (!yaml_valid_utf8(tag, strlen((char *)tag)))
            goto error;
        tag_copy = yaml_strdup(tag);
        if (!tag_copy)
            goto error;
    }

    MAPPING_START_EVENT_INIT(*event, anchor_copy, tag_copy,
            is_implicit, style, mark, mark);

    return 1;

error:
    yaml_free(anchor_copy);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Create MAPPING-END.
 */

YAML_DECLARE(int)
yaml_event_create_mapping_end(yaml_event_t *event)
{
    yaml_mark_t mark = { 0, 0, 0 };

    assert(event);      /* Non-NULL event object is expected. */

    MAPPING_END_EVENT_INIT(*event, mark, mark);

    return 1;
}

/*
 * Destroy an event object.
 */

YAML_DECLARE(void)
yaml_event_destroy(yaml_event_t *event)
{
    struct {
        yaml_error_t error;
    } self;

    assert(event);  /* Non-NULL event object expected. */

    switch (event->type)
    {
        case YAML_DOCUMENT_START_EVENT:
            yaml_free(event->data.document_start.version_directive);
            while (!STACK_EMPTY(&self, event->data.document_start.tag_directives)) {
                yaml_tag_directive_t value = POP(&self,
                        event->data.document_start.tag_directives);
                yaml_free(value.handle);
                yaml_free(value.prefix);
            }
            STACK_DEL(&self, event->data.document_start.tag_directives);
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

#if 0

/*
 * Create a document object.
 */

YAML_DECLARE(int)
yaml_document_initialize(yaml_document_t *document,
        yaml_version_directive_t *version_directive,
        yaml_tag_directive_t *tag_directives_start,
        yaml_tag_directive_t *tag_directives_end,
        int start_implicit, int end_implicit)
{
    struct {
        yaml_error_type_t error;
    } context;
    struct {
        yaml_node_t *start;
        yaml_node_t *end;
        yaml_node_t *top;
    } nodes = { NULL, NULL, NULL };
    yaml_version_directive_t *version_directive_copy = NULL;
    struct {
        yaml_tag_directive_t *start;
        yaml_tag_directive_t *end;
        yaml_tag_directive_t *top;
    } tag_directives_copy = { NULL, NULL, NULL };
    yaml_tag_directive_t value = { NULL, NULL };
    yaml_mark_t mark = { 0, 0, 0 };

    assert(document);       /* Non-NULL document object is expected. */
    assert((tag_directives_start && tag_directives_end) ||
            (tag_directives_start == tag_directives_end));
                            /* Valid tag directives are expected. */

    if (!STACK_INIT(&context, nodes, INITIAL_STACK_SIZE)) goto error;

    if (version_directive) {
        version_directive_copy = yaml_malloc(sizeof(yaml_version_directive_t));
        if (!version_directive_copy) goto error;
        version_directive_copy->major = version_directive->major;
        version_directive_copy->minor = version_directive->minor;
    }

    if (tag_directives_start != tag_directives_end) {
        yaml_tag_directive_t *tag_directive;
        if (!STACK_INIT(&context, tag_directives_copy, INITIAL_STACK_SIZE))
            goto error;
        for (tag_directive = tag_directives_start;
                tag_directive != tag_directives_end; tag_directive ++) {
            assert(tag_directive->handle);
            assert(tag_directive->prefix);
            if (!yaml_valid_utf8(tag_directive->handle,
                        strlen((char *)tag_directive->handle)))
                goto error;
            if (!yaml_valid_utf8(tag_directive->prefix,
                        strlen((char *)tag_directive->prefix)))
                goto error;
            value.handle = yaml_strdup(tag_directive->handle);
            value.prefix = yaml_strdup(tag_directive->prefix);
            if (!value.handle || !value.prefix) goto error;
            if (!PUSH(&context, tag_directives_copy, value))
                goto error;
            value.handle = NULL;
            value.prefix = NULL;
        }
    }

    DOCUMENT_INIT(*document, nodes.start, nodes.end, version_directive_copy,
            tag_directives_copy.start, tag_directives_copy.top,
            start_implicit, end_implicit, mark, mark);

    return 1;

error:
    STACK_DEL(&context, nodes);
    yaml_free(version_directive_copy);
    while (!STACK_EMPTY(&context, tag_directives_copy)) {
        yaml_tag_directive_t value = POP(&context, tag_directives_copy);
        yaml_free(value.handle);
        yaml_free(value.prefix);
    }
    STACK_DEL(&context, tag_directives_copy);
    yaml_free(value.handle);
    yaml_free(value.prefix);

    return 0;
}

/*
 * Destroy a document object.
 */

YAML_DECLARE(void)
yaml_document_delete(yaml_document_t *document)
{
    struct {
        yaml_error_type_t error;
    } context;
    yaml_tag_directive_t *tag_directive;

    context.error = YAML_NO_ERROR;  /* Eliminate a compliler warning. */

    assert(document);   /* Non-NULL document object is expected. */

    while (!STACK_EMPTY(&context, document->nodes)) {
        yaml_node_t node = POP(&context, document->nodes);
        yaml_free(node.tag);
        switch (node.type) {
            case YAML_SCALAR_NODE:
                yaml_free(node.data.scalar.value);
                break;
            case YAML_SEQUENCE_NODE:
                STACK_DEL(&context, node.data.sequence.items);
                break;
            case YAML_MAPPING_NODE:
                STACK_DEL(&context, node.data.mapping.pairs);
                break;
            default:
                assert(0);  /* Should not happen. */
        }
    }
    STACK_DEL(&context, document->nodes);

    yaml_free(document->version_directive);
    for (tag_directive = document->tag_directives.start;
            tag_directive != document->tag_directives.end;
            tag_directive++) {
        yaml_free(tag_directive->handle);
        yaml_free(tag_directive->prefix);
    }
    yaml_free(document->tag_directives.start);

    memset(document, 0, sizeof(yaml_document_t));
}

/**
 * Get a document node.
 */

YAML_DECLARE(yaml_node_t *)
yaml_document_get_node(yaml_document_t *document, int index)
{
    assert(document);   /* Non-NULL document object is expected. */

    if (index > 0 && document->nodes.start + index <= document->nodes.top) {
        return document->nodes.start + index - 1;
    }
    return NULL;
}

/**
 * Get the root object.
 */

YAML_DECLARE(yaml_node_t *)
yaml_document_get_root_node(yaml_document_t *document)
{
    assert(document);   /* Non-NULL document object is expected. */

    if (document->nodes.top != document->nodes.start) {
        return document->nodes.start;
    }
    return NULL;
}

/*
 * Add a scalar node to a document.
 */

YAML_DECLARE(int)
yaml_document_add_scalar(yaml_document_t *document,
        yaml_char_t *tag, yaml_char_t *value, int length,
        yaml_scalar_style_t style)
{
    struct {
        yaml_error_type_t error;
    } context;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *tag_copy = NULL;
    yaml_char_t *value_copy = NULL;
    yaml_node_t node;

    assert(document);   /* Non-NULL document object is expected. */
    assert(value);      /* Non-NULL value is expected. */

    if (!tag) {
        tag = (yaml_char_t *)YAML_DEFAULT_SCALAR_TAG;
    }

    if (!yaml_valid_utf8(tag, strlen((char *)tag))) goto error;
    tag_copy = yaml_strdup(tag);
    if (!tag_copy) goto error;

    if (length < 0) {
        length = strlen((char *)value);
    }

    if (!yaml_valid_utf8(value, length)) goto error;
    value_copy = yaml_malloc(length+1);
    if (!value_copy) goto error;
    memcpy(value_copy, value, length);
    value_copy[length] = '\0';

    SCALAR_NODE_INIT(node, tag_copy, value_copy, length, style, mark, mark);
    if (!PUSH(&context, document->nodes, node)) goto error;

    return document->nodes.top - document->nodes.start;

error:
    yaml_free(tag_copy);
    yaml_free(value_copy);

    return 0;
}

/*
 * Add a sequence node to a document.
 */

YAML_DECLARE(int)
yaml_document_add_sequence(yaml_document_t *document,
        yaml_char_t *tag, yaml_sequence_style_t style)
{
    struct {
        yaml_error_type_t error;
    } context;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *tag_copy = NULL;
    struct {
        yaml_node_item_t *start;
        yaml_node_item_t *end;
        yaml_node_item_t *top;
    } items = { NULL, NULL, NULL };
    yaml_node_t node;

    assert(document);   /* Non-NULL document object is expected. */

    if (!tag) {
        tag = (yaml_char_t *)YAML_DEFAULT_SEQUENCE_TAG;
    }

    if (!yaml_valid_utf8(tag, strlen((char *)tag))) goto error;
    tag_copy = yaml_strdup(tag);
    if (!tag_copy) goto error;

    if (!STACK_INIT(&context, items, INITIAL_STACK_SIZE)) goto error;

    SEQUENCE_NODE_INIT(node, tag_copy, items.start, items.end,
            style, mark, mark);
    if (!PUSH(&context, document->nodes, node)) goto error;

    return document->nodes.top - document->nodes.start;

error:
    STACK_DEL(&context, items);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Add a mapping node to a document.
 */

YAML_DECLARE(int)
yaml_document_add_mapping(yaml_document_t *document,
        yaml_char_t *tag, yaml_mapping_style_t style)
{
    struct {
        yaml_error_type_t error;
    } context;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *tag_copy = NULL;
    struct {
        yaml_node_pair_t *start;
        yaml_node_pair_t *end;
        yaml_node_pair_t *top;
    } pairs = { NULL, NULL, NULL };
    yaml_node_t node;

    assert(document);   /* Non-NULL document object is expected. */

    if (!tag) {
        tag = (yaml_char_t *)YAML_DEFAULT_MAPPING_TAG;
    }

    if (!yaml_valid_utf8(tag, strlen((char *)tag))) goto error;
    tag_copy = yaml_strdup(tag);
    if (!tag_copy) goto error;

    if (!STACK_INIT(&context, pairs, INITIAL_STACK_SIZE)) goto error;

    MAPPING_NODE_INIT(node, tag_copy, pairs.start, pairs.end,
            style, mark, mark);
    if (!PUSH(&context, document->nodes, node)) goto error;

    return document->nodes.top - document->nodes.start;

error:
    STACK_DEL(&context, pairs);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Append an item to a sequence node.
 */

YAML_DECLARE(int)
yaml_document_append_sequence_item(yaml_document_t *document,
        int sequence, int item)
{
    struct {
        yaml_error_type_t error;
    } context;

    assert(document);       /* Non-NULL document is required. */
    assert(sequence > 0
            && document->nodes.start + sequence <= document->nodes.top);
                            /* Valid sequence id is required. */
    assert(document->nodes.start[sequence-1].type == YAML_SEQUENCE_NODE);
                            /* A sequence node is required. */
    assert(item > 0 && document->nodes.start + item <= document->nodes.top);
                            /* Valid item id is required. */

    if (!PUSH(&context,
                document->nodes.start[sequence-1].data.sequence.items, item))
        return 0;

    return 1;
}

/*
 * Append a pair of a key and a value to a mapping node.
 */

YAML_DECLARE(int)
yaml_document_append_mapping_pair(yaml_document_t *document,
        int mapping, int key, int value)
{
    struct {
        yaml_error_type_t error;
    } context;
    yaml_node_pair_t pair = { key, value };

    assert(document);       /* Non-NULL document is required. */
    assert(mapping > 0
            && document->nodes.start + mapping <= document->nodes.top);
                            /* Valid mapping id is required. */
    assert(document->nodes.start[mapping-1].type == YAML_MAPPING_NODE);
                            /* A mapping node is required. */
    assert(key > 0 && document->nodes.start + key <= document->nodes.top);
                            /* Valid key id is required. */
    assert(value > 0 && document->nodes.start + value <= document->nodes.top);
                            /* Valid value id is required. */

    if (!PUSH(&context,
                document->nodes.start[mapping-1].data.mapping.pairs, pair))
        return 0;

    return 1;
}

#endif

