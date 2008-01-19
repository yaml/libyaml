/*****************************************************************************
 * LibYAML API Implementation
 *
 * Copyright (c) 2006 Kirill Simonov
 *
 * LibYAML is free software; you can use, modify and/or redistribute it under
 * the terms of the MIT license; see the file LICENCE for more details.
 *****************************************************************************/

#include "yaml_private.h"

/*****************************************************************************
 * Version Information
 *****************************************************************************/

/*
 * Get the library version as a static string.
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

/*****************************************************************************
 * Memory Management
 *****************************************************************************/

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

/*****************************************************************************
 * Error Handling
 *****************************************************************************/

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
                        "%s: %s at byte %d",
                        prefixes[error->type],
                        error->data.reading.problem,
                        error->data.reading.offset);
            }
            else {
                length = snprintf(buffer, capacity,
                        "%s: %s (#%X) at byte %d",
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
                    "%s: %s at byte %d",
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

/*****************************************************************************
 * String, Stack and Queue Management
 *****************************************************************************/

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

/*****************************************************************************
 * Token API
 *****************************************************************************/

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

    yaml_token_clear(token);
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
    assert(!token->type);   /* The token must be empty. */

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
            token->data.scalar.style = model->data.scalar.style;
            break;

        default:
            break;
    }

    return 1;

error:
    yaml_token_clear(token);

    return 0;
}

/*
 * Clear a token object.
 */

YAML_DECLARE(void)
yaml_token_clear(yaml_token_t *token)
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

/*****************************************************************************
 * Event API
 *****************************************************************************/

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

    yaml_event_clear(event);
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
    assert(!event->type);   /* The event must be empty. */

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
            break;

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
            if (model->data.scalar.tag &&
                    !(event->data.scalar.tag =
                        yaml_strdup(model->data.scalar.tag)))
                goto error;
            if (!(event->data.scalar.value =
                        yaml_malloc(model->data.scalar.length+1)))
                goto error;
            memcpy(event->data.scalar.value, model->data.scalar.value,
                    model->data.scalar.length+1);
            event->data.scalar.length = model->data.scalar.length;
            event->data.scalar.is_plain_nonspecific =
                model->data.scalar.is_plain_nonspecific;
            event->data.scalar.is_quoted_nonspecific =
                model->data.scalar.is_quoted_nonspecific;
            event->data.scalar.style = model->data.scalar.style;
            break;

        case YAML_SEQUENCE_START_EVENT:
            if (model->data.sequence_start.anchor &&
                    !(event->data.sequence_start.anchor =
                        yaml_strdup(model->data.sequence_start.anchor)))
                goto error;
            if (model->data.sequence_start.tag &&
                    !(event->data.sequence_start.tag =
                        yaml_strdup(model->data.sequence_start.tag)))
                goto error;
            event->data.sequence_start.is_nonspecific =
                model->data.sequence_start.is_nonspecific;
            event->data.sequence_start.style =
                model->data.sequence_start.style;
            break;

        case YAML_MAPPING_START_EVENT:
            if (model->data.mapping_start.anchor &&
                    !(event->data.mapping_start.anchor =
                        yaml_strdup(model->data.mapping_start.anchor)))
                goto error;
            if (model->data.mapping_start.tag &&
                    !(event->data.mapping_start.tag =
                        yaml_strdup(model->data.mapping_start.tag)))
                goto error;
            event->data.mapping_start.is_nonspecific =
                model->data.mapping_start.is_nonspecific;
            event->data.mapping_start.style =
                model->data.mapping_start.style;
            break;

        default:
            break;
    }

    return 1;

error:
    yaml_event_clear(event);

    return 0;
}

/*
 * Clear an event object.
 */

YAML_DECLARE(void)
yaml_event_clear(yaml_event_t *event)
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

/*
 * Create STREAM-START.
 */

YAML_DECLARE(int)
yaml_event_create_stream_start(yaml_event_t *event,
        yaml_encoding_t encoding)
{
    yaml_mark_t mark = { 0, 0, 0 };

    assert(event);  /* Non-NULL event object is expected. */
    assert(!event->type);   /* The event must be empty. */

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
    assert(!event->type);   /* The event must be empty. */

    STREAM_END_EVENT_INIT(*event, mark, mark);

    return 1;
}

/*
 * Create DOCUMENT-START.
 */

YAML_DECLARE(int)
yaml_event_create_document_start(yaml_event_t *event,
        const yaml_version_directive_t *version_directive,
        const yaml_tag_directive_t *tag_directives_list,
        size_t tag_directives_length, int is_implicit)
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
    int idx;

    assert(event);          /* Non-NULL event object is expected. */
    assert(!event->type);   /* The event must be empty. */

    if (version_directive) {
        version_directive_copy = yaml_malloc(sizeof(yaml_version_directive_t));
        if (!version_directive_copy) goto error;
        *version_directive_copy = *version_directive;
    }

    if (tag_directives_list && tag_directives_length) {
        if (!STACK_INIT(&self, tag_directives_copy, tag_directives_length))
            goto error;
        for (idx = 0; idx < tag_directives_length; idx++) {
            yaml_tag_directive_t value = tag_directives_list[idx];
            assert(value.handle);
            assert(value.prefix);
            value.handle = yaml_strdup(value.handle);
            value.prefix = yaml_strdup(value.prefix);
            PUSH(&self, tag_directives_copy, value);
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
    assert(!event->type);   /* The event must be empty. */

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
    assert(!event->type);   /* The event must be empty. */
    assert(anchor);     /* Non-NULL anchor is expected. */

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
        const yaml_char_t *value, int length,
        int is_plain_nonspecific, int is_quoted_nonspecific,
        yaml_scalar_style_t style)
{
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;
    yaml_char_t *value_copy = NULL;

    assert(event);      /* Non-NULL event object is expected. */
    assert(!event->type);   /* The event must be empty. */
    assert(value);      /* Non-NULL anchor is expected. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy)
            goto error;
    }

    if (tag) {
        tag_copy = yaml_strdup(tag);
        if (!tag_copy)
            goto error;
    }

    if (length < 0) {
        length = strlen((char *)value);
    }

    value_copy = yaml_malloc(length+1);
    if (!value_copy)
        goto error;
    memcpy(value_copy, value, length);
    value_copy[length] = '\0';

    SCALAR_EVENT_INIT(*event, anchor_copy, tag_copy, value_copy, length,
            is_plain_nonspecific, is_quoted_nonspecific, style, mark, mark);

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
        int is_nonspecific, yaml_sequence_style_t style)
{
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;

    assert(event);      /* Non-NULL event object is expected. */
    assert(!event->type);   /* The event must be empty. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy)
            goto error;
    }

    if (tag) {
        tag_copy = yaml_strdup(tag);
        if (!tag_copy)
            goto error;
    }

    SEQUENCE_START_EVENT_INIT(*event, anchor_copy, tag_copy,
            is_nonspecific, style, mark, mark);

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
    assert(!event->type);   /* The event must be empty. */

    SEQUENCE_END_EVENT_INIT(*event, mark, mark);

    return 1;
}

/*
 * Create MAPPING-START.
 */

YAML_DECLARE(int)
yaml_event_create_mapping_start(yaml_event_t *event,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        int is_nonspecific, yaml_mapping_style_t style)
{
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;

    assert(event);      /* Non-NULL event object is expected. */
    assert(!event->type);   /* The event must be empty. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy)
            goto error;
    }

    if (tag) {
        tag_copy = yaml_strdup(tag);
        if (!tag_copy)
            goto error;
    }

    MAPPING_START_EVENT_INIT(*event, anchor_copy, tag_copy,
            is_nonspecific, style, mark, mark);

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
    assert(!event->type);   /* The event must be empty. */

    MAPPING_END_EVENT_INIT(*event, mark, mark);

    return 1;
}

/*****************************************************************************
 * Document API
 *****************************************************************************/

/*
 * Allocate a document object.
 */

YAML_DECLARE(yaml_document_t *)
yaml_document_new(void)
{
    yaml_document_t *document = yaml_malloc(sizeof(yaml_document_t));

    if (!document)
        return NULL;

    memset(document, 0, sizeof(yaml_document_t));

    return document;
}

/*
 * Deallocate a document object.
 */

YAML_DECLARE(void)
yaml_document_delete(yaml_document_t *document)
{
    assert(document);   /* Non-NULL document object is expected. */

    yaml_document_clear(document);
    yaml_free(document);
}

/*
 * Duplicate a document object.
 */

YAML_DECLARE(int)
yaml_document_duplicate(yaml_document_t *document, yaml_document_t *model)
{
    struct {
        yaml_error_t error;
    } self;
    yaml_char_t *anchor = NULL;
    yaml_char_t *tag = NULL;
    yaml_char_t *value = NULL;
    yaml_node_item_t *item_list = NULL;
    yaml_node_pair_t *pair_list = NULL;
    int idx;

    assert(document);   /* Non-NULL document object is expected. */
    assert(!document->type);    /* The document must be empty. */
    assert(model);      /* Non-NULL model object is expected. */

    if (model->type != YAML_DOCUMENT)
        return 1;

    if (!yaml_document_create(document, model->version_directive,
                model->tag_directives.list, model->tag_directives.length,
                model->is_start_implicit, model->is_end_implicit))
        return 0;

    document->start_mark = model->start_mark;
    document->end_mark = model->end_mark;

    for (idx = 0; idx < model->nodes.length; idx++)
    {
        yaml_node_t *node = STACK_ITER(&self, model->nodes, idx);
        yaml_node_t copy;
        if (node->anchor) {
            anchor = yaml_strdup(node->anchor);
            if (!anchor) goto error;
        }
        tag = yaml_strdup(node->tag);
        if (!tag) goto error;
        switch (node->type)
        {
            case YAML_SCALAR_NODE:
                value = yaml_malloc(node->data.scalar.length+1);
                if (!value)
                    goto error;
                memcpy(value, node->data.scalar.value,
                        node->data.scalar.length);
                value[node->data.scalar.length] = '\0';
                SCALAR_NODE_INIT(copy, anchor, tag, value,
                        node->data.scalar.length, node->data.scalar.style,
                        node->start_mark, node->end_mark);
                break;

            case YAML_SEQUENCE_NODE:
                item_list = yaml_malloc(node->data.sequence.items.capacity
                        * sizeof(yaml_node_item_t));
                if (!item_list) goto error;
                memcpy(item_list, node->data.sequence.items.list,
                        node->data.sequence.items.capacity
                        * sizeof(yaml_node_item_t));
                SEQUENCE_NODE_INIT(copy, anchor, tag, item_list,
                        node->data.sequence.items.length,
                        node->data.sequence.items.capacity,
                        node->data.sequence.style,
                        node->start_mark, node->end_mark);
                break;

            case YAML_MAPPING_NODE:
                pair_list = yaml_malloc(node->data.mapping.pairs.capacity
                        * sizeof(yaml_node_pair_t));
                if (!pair_list) goto error;
                memcpy(pair_list, node->data.mapping.pairs.list,
                        node->data.mapping.pairs.capacity
                        * sizeof(yaml_node_pair_t));
                MAPPING_NODE_INIT(copy, anchor, tag, pair_list,
                        node->data.mapping.pairs.length,
                        node->data.mapping.pairs.capacity,
                        node->data.mapping.style,
                        node->start_mark, node->end_mark);
                break;

            default:
                assert(0);  /* Should never happen. */
        }

        if (!PUSH(&self, document->nodes, copy))
            goto error;

        anchor = NULL;
        tag = NULL;
        value = NULL;
        item_list = NULL;
        pair_list = NULL;
    }

error:
    yaml_free(anchor);
    yaml_free(tag);
    yaml_free(value);
    yaml_free(item_list);
    yaml_free(pair_list);

    yaml_document_clear(document);

    return 0;
}

/*
 * Clear a document object.
 */

YAML_DECLARE(void)
yaml_document_clear(yaml_document_t *document)
{
    struct {
        yaml_error_type_t error;
    } self;
    yaml_tag_directive_t *tag_directive;

    self.error = YAML_NO_ERROR;  /* Eliminate a compliler warning. */

    assert(document);   /* Non-NULL document object is expected. */

    if (!document->type)
        return;

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
    STACK_DEL(&self, document->nodes);

    yaml_free(document->version_directive);
    while (!STACK_EMPTY(&self, document->tag_directives)) {
        yaml_tag_directive_t tag_directive = POP(&self, document->tag_directives);
        yaml_free(tag_directive.handle);
        yaml_free(tag_directive.prefix);
    }
    STACK_DEL(&self, document->tag_directives);

    memset(document, 0, sizeof(yaml_document_t));
}

/*
 * Create a document.
 */

YAML_DECLARE(int)
yaml_document_create(yaml_document_t *document,
        const yaml_version_directive_t *version_directive,
        const yaml_tag_directive_t *tag_directives_list,
        size_t tag_directives_length,
        int is_start_implicit, int is_end_implicit)
{
    struct {
        yaml_error_t error;
    } self;
    yaml_mark_t mark = { 0, 0, 0 };
    struct {
        yaml_node_t *list;
        size_t length;
        size_t capacity;
    } nodes;
    yaml_version_directive_t *version_directive_copy = NULL;
    struct {
        yaml_tag_directive_t *list;
        size_t length;
        size_t capacity;
    } tag_directives_copy = { NULL, 0, 0 };
    int idx;

    assert(document);           /* Non-NULL event object is expected. */
    assert(!document->type);    /* The document must be empty. */
    
    if (!STACK_INIT(&self, nodes, INITIAL_STACK_CAPACITY))
        goto error;

    if (version_directive) {
        version_directive_copy = yaml_malloc(sizeof(yaml_version_directive_t));
        if (!version_directive_copy) goto error;
        *version_directive_copy = *version_directive;
    }

    if (tag_directives_list && tag_directives_length) {
        if (!STACK_INIT(&self, tag_directives_copy, tag_directives_length))
            goto error;
        for (idx = 0; idx < tag_directives_length; idx++) {
            yaml_tag_directive_t value = tag_directives_list[idx];
            assert(value.handle);
            assert(value.prefix);
            value.handle = yaml_strdup(value.handle);
            value.prefix = yaml_strdup(value.prefix);
            PUSH(&self, tag_directives_copy, value);
            if (!value.handle || !value.prefix)
                goto error;
        }
    }

    DOCUMENT_INIT(*document, nodes.list, nodes.length, nodes.capacity,
            version_directive_copy, tag_directives_copy.list,
            tag_directives_copy.length, tag_directives_copy.capacity,
            is_start_implicit, is_end_implicit, mark, mark);

    return 1;

error:
    STACK_DEL(&self, nodes);

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
 * Get a document node.
 */

YAML_DECLARE(yaml_node_t *)
yaml_document_get_node(yaml_document_t *document, int node_id)
{
    assert(document);   /* Non-NULL document object is expected. */
    assert(document->type); /* Initialized document is expected. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    if (node_id >= 0 && node_id < document->nodes.length) {
        return document->nodes.list + node_id;
    }
    return NULL;
}

/*
 * Add a scalar node to a document.
 */

YAML_DECLARE(int)
yaml_document_add_scalar(yaml_document_t *document, int *node_id,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        const yaml_char_t *value, int length,
        yaml_scalar_style_t style)
{
    struct {
        yaml_error_t error;
    } self;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;
    yaml_char_t *value_copy = NULL;
    yaml_node_t node;

    assert(document);   /* Non-NULL document object is expected. */
    assert(document->type); /* Initialized document is required. */
    assert(tag);        /* Non-NULL tag is expected. */
    assert(value);      /* Non-NULL value is expected. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy) goto error;
    }

    tag_copy = yaml_strdup(tag);
    if (!tag_copy) goto error;

    if (length < 0) {
        length = strlen((char *)value);
    }

    value_copy = yaml_malloc(length+1);
    if (!value_copy) goto error;
    memcpy(value_copy, value, length);
    value_copy[length] = '\0';

    SCALAR_NODE_INIT(node, anchor_copy, tag_copy, value_copy, length,
            style, mark, mark);
    if (!PUSH(&self, document->nodes, node)) goto error;

    if (node_id) {
        *node_id = document->nodes.length-1;
    }

    return 1;

error:
    yaml_free(anchor_copy);
    yaml_free(tag_copy);
    yaml_free(value_copy);

    return 0;
}

/*
 * Add a sequence node to a document.
 */

YAML_DECLARE(int)
yaml_document_add_sequence(yaml_document_t *document, int *node_id,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        yaml_sequence_style_t style)
{
    struct {
        yaml_error_t error;
    } self;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;
    struct {
        yaml_node_item_t *list;
        size_t length;
        size_t capacity;
    } items = { NULL, 0, 0 };
    yaml_node_t node;

    assert(document);   /* Non-NULL document object is expected. */
    assert(document->type); /* Initialized document is required. */
    assert(tag);        /* Non-NULL tag is expected. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy) goto error;
    }

    tag_copy = yaml_strdup(tag);
    if (!tag_copy) goto error;

    if (!STACK_INIT(&self, items, INITIAL_STACK_CAPACITY)) goto error;

    SEQUENCE_NODE_INIT(node, anchor_copy, tag_copy,
            items.list, items.length, items.capacity, style, mark, mark);
    if (!PUSH(&self, document->nodes, node)) goto error;

    if (node_id) {
        *node_id = document->nodes.length-1;
    }

    return 1;

error:
    STACK_DEL(&self, items);
    yaml_free(anchor_copy);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Add a mapping node to a document.
 */

YAML_DECLARE(int)
yaml_document_add_mapping(yaml_document_t *document, int *node_id,
        const yaml_char_t *anchor, const yaml_char_t *tag,
        yaml_mapping_style_t style)
{
    struct {
        yaml_error_t error;
    } self;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;
    struct {
        yaml_node_pair_t *list;
        size_t length;
        size_t capacity;
    } pairs = { NULL, 0, 0 };
    yaml_node_t node;

    assert(document);   /* Non-NULL document object is expected. */
    assert(document->type); /* Initialized document is required. */
    assert(tag);        /* Non-NULL tag is expected. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy) goto error;
    }

    tag_copy = yaml_strdup(tag);
    if (!tag_copy) goto error;

    if (!STACK_INIT(&self, pairs, INITIAL_STACK_CAPACITY)) goto error;

    MAPPING_NODE_INIT(node, anchor_copy, tag_copy,
            pairs.list, pairs.length, pairs.capacity, style, mark, mark);
    if (!PUSH(&self, document->nodes, node)) goto error;

    if (node_id) {
        *node_id = document->nodes.length-1;
    }

    return 1;

error:
    STACK_DEL(&self, pairs);
    yaml_free(anchor_copy);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Append an item to a sequence node.
 */

YAML_DECLARE(int)
yaml_document_append_sequence_item(yaml_document_t *document,
        int sequence_id, int item_id)
{
    struct {
        yaml_error_t error;
    } self;

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    if (sequence_id) {
        sequence_id += document->nodes.length;
    }
    if (item_id) {
        item_id += document->nodes.length;
    }

    assert(sequence_id >= 0 && sequence_id < document->nodes.length);
                            /* Valid sequence id is required. */
    assert(item_id >= 0 && item_id < document->nodes.length);
                            /* Valid item id is required. */
    assert(document->nodes.list[sequence_id].type == YAML_SEQUENCE_NODE);
                            /* A sequence node is expected. */

    if (!PUSH(&self,
                document->nodes.list[sequence_id].data.sequence.items, item_id))
        return 0;

    return 1;
}

/*
 * Append a pair of a key and a value to a mapping node.
 */

YAML_DECLARE(int)
yaml_document_append_mapping_pair(yaml_document_t *document,
        int mapping_id, int key_id, int value_id)
{
    struct {
        yaml_error_t error;
    } self;
    yaml_node_pair_t pair = { key_id, value_id };

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    if (mapping_id < 0) {
        mapping_id += document->nodes.length;
    }
    if (key_id < 0) {
        key_id += document->nodes.length;
    }
    if (value_id < 0) {
        value_id += document->nodes.length;
    }

    assert(mapping_id >= 0 && mapping_id < document->nodes.length);
                            /* Valid mapping id is required. */
    assert(key_id >= 0 && key_id < document->nodes.length);
                            /* Valid key id is required. */
    assert(value_id >= 0 && value_id < document->nodes.length);
                            /* Valid value id is required. */
    assert(document->nodes.list[mapping_id].type == YAML_MAPPING_NODE);
                            /* A mapping node is expected. */

    if (!PUSH(&self,
                document->nodes.list[mapping_id].data.mapping.pairs, pair))
        return 0;

    return 1;
}

/*
 * Ensure that the node is a `!!null` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_get_null_node(yaml_document_t *document, int node_id)
{
    yaml_node_t *node;
    yaml_char_t *scalar;

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    assert(node_id >= 0 && node_id < document->nodes.length);
                            /* Valid node id is required. */

    node = document->nodes.list + node_id;

    if (node->type != YAML_SCALAR_NODE)
        return 0;

    if (strcmp(node->tag, YAML_NULL_TAG))
        return 0;

    if (node->data.scalar.length != strlen(node->data.scalar.value))
        return 0;

    scalar = node->data.scalar.value;

    if (!strcmp(scalar, "") || !strcmp(scalar, "~") || !strcmp(scalar, "null")
            || !strcmp(scalar, "Null") || !strcmp(scalar, "NULL"))
        return 1;

    return 0;
}

/*
 * Ensure that the node is a `!!bool` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_get_bool_node(yaml_document_t *document, int node_id, int *value)
{
    yaml_node_t *node;
    yaml_char_t *scalar;

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    assert(node_id >= 0 && node_id < document->nodes.length);
                            /* Valid node id is required. */

    node = document->nodes.list + node_id;

    if (node->type != YAML_SCALAR_NODE)
        return 0;

    if (strcmp(node->tag, YAML_BOOL_TAG))
        return 0;

    if (node->data.scalar.length != strlen(node->data.scalar.value))
        return 0;

    scalar = node->data.scalar.value;

    if (!strcmp(scalar, "yes") || !strcmp(scalar, "Yes") || !strcmp(scalar, "YES") ||
            !strcmp(scalar, "true") || !strcmp(scalar, "True") || !strcmp(scalar, "TRUE") ||
            !strcmp(scalar, "on") || !strcmp(scalar, "On") || !strcmp(scalar, "ON")) {
        if (value) {
            *value = 1;
        }
        return 1;
    }

    if (!strcmp(scalar, "no") || !strcmp(scalar, "No") || !strcmp(scalar, "NO") ||
            !strcmp(scalar, "false") || !strcmp(scalar, "False") || !strcmp(scalar, "FALSE") ||
            !strcmp(scalar, "off") || !strcmp(scalar, "Off") || !strcmp(scalar, "OFF")) {
        if (value) {
            *value = 0;
        }
        return 1;
    }

    return 0;
}

/*
 * Ensure that the node is a `!!str` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_get_str_node(yaml_document_t *document, int node_id,
        char **value)
{
    yaml_node_t *node;

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    assert(node_id >= 0 && node_id < document->nodes.length);
                            /* Valid node id is required. */

    node = document->nodes.list + node_id;

    if (node->type != YAML_SCALAR_NODE)
        return 0;

    if (strcmp(node->tag, YAML_STR_TAG))
        return 0;

    if (node->data.scalar.length != strlen(node->data.scalar.value))
        return 0;

    if (value) {
        *value = node->data.scalar.value;
    }

    return 1;
}

/*
 * Ensure that the node is an `!!int` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_get_int_node(yaml_document_t *document, int node_id,
        long *value)
{
    yaml_node_t *node;
    yaml_char_t *scalar;
    char *tail;
    long integer;
    int old_errno = errno, new_errno;

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    assert(node_id >= 0 && node_id < document->nodes.length);
                            /* Valid node id is required. */

    node = document->nodes.list + node_id;

    if (node->type != YAML_SCALAR_NODE)
        return 0;

    if (strcmp(node->tag, YAML_INT_TAG))
        return 0;

    if (node->data.scalar.length != strlen(node->data.scalar.value))
        return 0;

    if (!node->data.scalar.length)
        return 0;

    scalar = node->data.scalar.value;

    errno = 0;

    integer = strtol((char *)scalar, &tail, 0);

    new_errno = errno;
    errno = old_errno;

    if (new_errno || *tail) {
        return 0;
    }

    if (value) {
        *value = integer;
    }

    return 1;
}

/*
 * Ensure that the node is a `!!float` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_get_float_node(yaml_document_t *document, int node_id,
        double *value)
{
    yaml_node_t *node;
    yaml_char_t *scalar;
    char buffer[128];
    char *pointer;
    char *tail;
    double real;
    int old_errno = errno, new_errno;
    char decimal_point = localeconv()->decimal_point[0];

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    assert(node_id >= 0 && node_id < document->nodes.length);
                            /* Valid node id is required. */

    node = document->nodes.list + node_id;

    if (node->type != YAML_SCALAR_NODE)
        return 0;

    if (strcmp(node->tag, YAML_FLOAT_TAG) && strcmp(node->tag, YAML_INT_TAG))
        return 0;

    if (node->data.scalar.length != strlen(node->data.scalar.value))
        return 0;

    if (!node->data.scalar.length)
        return 0;

    scalar = node->data.scalar.value;

    if (!strcmp(scalar, ".nan") || !strcmp(scalar, ".NaN") || !strcmp(scalar, ".NAN")) {
        if (value) {
            *value = 0.0/0.0;
        }
        return 1;
    }

    if (!strcmp(scalar, ".inf") || !strcmp(scalar, ".Inf") || !strcmp(scalar, ".INF") ||
            !strcmp(scalar, "+.inf") || !strcmp(scalar, "+.Inf") || !strcmp(scalar, "+.INF")) {
        if (value) {
            *value = 1.0/0.0;
        }
        return 1;
    }

    if (!strcmp(scalar, "-.inf") || !strcmp(scalar, "-.Inf") || !strcmp(scalar, "-.INF")) {
        if (value) {
            *value = -1.0/0.0;
        }
        return 1;
    }

    if (strlen(scalar) >= sizeof(buffer))
        return 0;

    strcpy(buffer, (const char *)scalar);

    /* Replace a locale-dependent decimal point with a dot. */

    for (pointer = buffer; *pointer; pointer++) {
        if (*pointer == decimal_point) {
            *pointer = '.';
            break;
        }
    }

    errno = 0;

    real = strtod(buffer, &tail);

    new_errno = errno;
    errno = old_errno;

    if (new_errno || *tail) {
        return 0;
    }

    if (value) {
        *value = real;
    }

    return 1;
}

/*
 * Ensure that the node is a `!!seq` SEQUENCE node.
 */

YAML_DECLARE(int)
yaml_document_get_seq_node(yaml_document_t *document, int node_id,
        yaml_node_item_t **items, size_t *length)
{
    yaml_node_t *node;

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    assert((items && length) || (!items && !length));
                /* items and length must be equal to NULL simultaneously. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    assert(node_id >= 0 && node_id < document->nodes.length);
                            /* Valid node id is required. */

    node = document->nodes.list + node_id;

    if (node->type != YAML_SEQUENCE_NODE)
        return 0;

    if (strcmp(node->tag, YAML_SEQ_TAG))
        return 0;

    if (items && length) {
        *items = node->data.sequence.items.list;
        *length = node->data.sequence.items.length;
    }

    return 1;
}

/*
 * Ensure that the node is a `!!map` MAPPING node.
 */

YAML_DECLARE(int)
yaml_document_get_map_node(yaml_document_t *document, int node_id,
        yaml_node_pair_t **pairs, size_t *length)
{
    yaml_node_t *node;

    assert(document);       /* Non-NULL document is required. */
    assert(document->type); /* Initialized document is expected. */

    assert((pairs && length) || (!pairs && !length));
                /* pairs and length must be equal to NULL simultaneously. */

    if (node_id < 0) {
        node_id += document->nodes.length;
    }

    assert(node_id >= 0 && node_id < document->nodes.length);
                            /* Valid node id is required. */

    node = document->nodes.list + node_id;

    if (node->type != YAML_MAPPING_NODE)
        return 0;

    if (strcmp(node->tag, YAML_MAP_TAG))
        return 0;

    if (pairs && length) {
        *pairs = node->data.mapping.pairs.list;
        *length = node->data.mapping.pairs.length;
    }

    return 1;
}

/*
 * Add a `!!null` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_add_null_node(yaml_document_t *document, int *node_id)
{
    return yaml_document_add_scalar(document, node_id, NULL,
            YAML_NULL_TAG, "null", -1, YAML_ANY_SCALAR_STYLE);
}

/*
 * Add a `!!bool` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_add_bool_node(yaml_document_t *document, int *node_id,
        int value)
{
    return yaml_document_add_scalar(document, node_id, NULL, YAML_BOOL_TAG,
            (value ? "true" : "false"), -1, YAML_ANY_SCALAR_STYLE);
}

/*
 * Add a `!!str` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_add_str_node(yaml_document_t *document, int *node_id,
        const char *value)
{
    return yaml_document_add_scalar(document, node_id, NULL, YAML_STR_TAG,
            (const yaml_char_t *) value, -1, YAML_ANY_SCALAR_STYLE);
}

/*
 * Add an `!!int` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_add_int_node(yaml_document_t *document, int *node_id,
        long value)
{
    char buffer[128];   /* 128 bytes should be enough for everybody. */
    int length;

    length = snprintf(buffer, sizeof(buffer), "%ld", value);
    if (length < 0 || length >= sizeof(buffer)) return 0;

    return yaml_document_add_scalar(document, node_id, NULL, YAML_INT_TAG,
            (const yaml_char_t *) buffer, -1, YAML_ANY_SCALAR_STYLE);
}

/*
 * Add a `!!float` SCALAR node.
 */

YAML_DECLARE(int)
yaml_document_add_float_node(yaml_document_t *document, int *node_id,
        double value)
{
    char buffer[128];   /* 128 bytes should be enough for everybody. */
    char *pointer;
    int length;
    char decimal_point = *localeconv()->decimal_point;

    length = snprintf(buffer, sizeof(buffer), "%.12g", value);
        /* .12 is a reasonable precision; it is used by str(float) in Python. */
    if (length < 0 || length >= sizeof(buffer)-3) return 0;

    /* Replace a locale-dependent decimal point with a dot. */

    for (pointer = buffer; *pointer; pointer++) {
        if (*pointer == decimal_point) {
            *pointer = '.';
            break;
        }
    }

    /* Check if the formatted number contains a decimal dot. */

    for (pointer = buffer; *pointer; pointer++) {
        if (*pointer != '+' && *pointer != '-' && !isdigit(*pointer)) {
            break;
        }
    }

    /* Add .0 at the end of the buffer if needed. */

    if (!*pointer) {
        *(pointer++) = '.';
        *(pointer++) = '0';
        *(pointer++) = '\0';
    }

    return yaml_document_add_scalar(document, node_id, NULL, YAML_FLOAT_TAG,
            (const yaml_char_t *) buffer, -1, YAML_ANY_SCALAR_STYLE);
}

/*
 * Add a `!!seq` SEQUENCE node.
 */

YAML_DECLARE(int)
yaml_document_add_seq_node(yaml_document_t *document, int *node_id)
{
    return yaml_document_add_sequence(document, node_id, NULL,
            YAML_SEQ_TAG, YAML_ANY_SEQUENCE_STYLE);
}

/*
 * Add a `!!map` MAPPING node.
 */

YAML_DECLARE(int)
yaml_document_add_map_node(yaml_document_t *document, int *node_id)
{
    return yaml_document_add_mapping(document, node_id, NULL,
            YAML_MAP_TAG, YAML_ANY_MAPPING_STYLE);
}

/*****************************************************************************
 * Standard Handlers
 *****************************************************************************/

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
 * Standard resolve handler.
 *
 * The standard resolve handler recognizes the following scalars:
 *
 * - `!!null`: `~|null|Null|NULL|<empty string>`.
 *
 * - `!!bool`: `yes|Yes|YES|no|No|NO|true|True|TRUE|false|False|FALSE|
 *              on|On|ON|off|Off|OFF`
 *
 * - `!!int`: any string that is successfully converted using `strtol()`.
 *
 * - `!!float`: `[+-]?(.inf|.Inf|.INF)|.nan|.NaN|.NAN` or any string
 *   successfully converted using `strtod()`.
 */

static int
yaml_standard_resolver(void *untyped_data, yaml_incomplete_node_t *node,
        const yaml_char_t **tag)
{
    if (node->type == YAML_SCALAR_NODE && node->data.scalar.is_plain)
    {
        yaml_char_t *value = node->data.scalar.value;
        char buffer[128];
        char *pointer;
        char *tail;
        int old_errno, new_errno;
        char decimal_point = *(localeconv()->decimal_point);

        if (strlen(value) != node->data.scalar.length) {
            *tag = YAML_STR_TAG;
            return 1;
        }

        if (!strcmp(value, "") || !strcmp(value, "~") ||
                !strcmp(value, "null") || !strcmp(value, "Null") || !strcmp(value, "NULL")) {
            *tag = YAML_NULL_TAG;
            return 1;
        }

        if (!strcmp(value, "yes") || !strcmp(value, "Yes") || !strcmp(value, "YES") ||
                !strcmp(value, "no") || !strcmp(value, "No") || !strcmp(value, "NO") ||
                !strcmp(value, "true") || !strcmp(value, "True") || !strcmp(value, "TRUE") ||
                !strcmp(value, "false") || !strcmp(value, "False") || !strcmp(value, "FALSE") ||
                !strcmp(value, "on") || !strcmp(value, "On") || !strcmp(value, "ON") ||
                !strcmp(value, "off") || !strcmp(value, "Off") || !strcmp(value, "OFF")) {
            *tag = YAML_BOOL_TAG;
            return 1;
        }

        if (!strcmp(value, ".inf") || !strcmp(value, ".Inf") || !strcmp(value, ".INF") ||
                !strcmp(value, "+.inf") || !strcmp(value, "+.Inf") || !strcmp(value, "+.INF") ||
                !strcmp(value, "-.inf") || !strcmp(value, "-.Inf") || !strcmp(value, "-.INF") ||
                !strcmp(value, ".nan") || !strcmp(value, ".NaN") || !strcmp(value, ".NAN")) {
            *tag = YAML_FLOAT_TAG;
            return 1;
        }

        old_errno = errno;
        errno = 0;

        strtol((const char *)value, &tail, 0);

        new_errno = errno;
        errno = old_errno;

        if (!new_errno && !*tail) {
            *tag = YAML_INT_TAG;
            return 1;
        }

        if (strlen(value) < sizeof(buffer))
        {
            strcpy(buffer, (const char *)value);

            /* Replace a locale-dependent decimal point with a dot. */

            for (pointer = buffer; *pointer; pointer++) {
                if (*pointer == decimal_point) {
                    *pointer = '.';
                    break;
                }
            }

            old_errno = errno;
            errno = 0;

            strtod(buffer, &tail);

            new_errno = errno;
            errno = old_errno;

            if (!new_errno && !*tail) {
                *tag = YAML_FLOAT_TAG;
                return 1;
            }
        }
    }

    switch (node->type)
    {
        case YAML_SCALAR_NODE:
            *tag = YAML_STR_TAG;
            break;
        case YAML_SEQUENCE_NODE:
            *tag = YAML_SEQ_TAG;
            break;
        case YAML_MAPPING_NODE:
            *tag = YAML_MAP_TAG;
            break;
        default:
            assert(0);      /* Should never happen. */
    }

    return 1;
}


/*****************************************************************************
 * Parser API
 *****************************************************************************/

/*
 * Allocate a new parser object.
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
 * Deallocate a parser object.
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
 * Reset a parser object.
 */

YAML_DECLARE(void)
yaml_parser_reset(yaml_parser_t *parser)
{
    yaml_parser_t copy = *parser;

    assert(parser); /* Non-NULL parser object expected. */

    while (!QUEUE_EMPTY(parser, parser->tokens)) {
        yaml_token_destroy(&DEQUEUE(parser, parser->tokens));
    }
    while (!STACK_EMPTY(parser, parser->tag_directives)) {
        yaml_tag_directive_t tag_directive = POP(parser, parser->tag_directives);
        yaml_free(tag_directive.handle);
        yaml_free(tag_directive.prefix);
    }

    memset(parser, 0, sizeof(yaml_parser_t));

    IOSTRING_SET(parser, parser->raw_input,
            copy.raw_input.buffer, copy.raw_input.capacity);
    IOSTRING_SET(parser, parser->input,
            copy.input.buffer, copy.input.capacity);
    QUEUE_SET(parser, parser->tokens,
            copy.tokens.list, copy.tokens.capacity);
    STACK_SET(parser, parser->indents,
            copy.indents.list, copy.indents.capacity);
    STACK_SET(parser, parser->simple_keys,
            copy.simple_keys.list, copy.simple_keys.capacity);
    STACK_SET(parser, parser->states,
            copy.states.list, copy.states.capacity);
    STACK_SET(parser, parser->marks,
            copy.marks.list, copy.marks.capacity);
    STACK_SET(parse, parser->tag_directives,
            copy.tag_directives.list, copy.tag_directives.capacity);
}

/*
 * Get the current parser error.
 */

YAML_DECLARE(yaml_error_t *)
yaml_parser_get_error(yaml_parser_t *parser)
{
    assert(parser); /* Non-NULL parser object expected. */

    return &(parser->error);
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
 * Set a standard tag resolver.
 */

YAML_DECLARE(void)
yaml_parser_set_standard_resolver(yaml_parser_t *parser)
{
    assert(parser); /* Non-NULL parser object expected. */
    assert(!parser->resolver);  /* You can set the tag resolver only once. */

    parser->resolver = yaml_standard_resolver;
    parser->resolver_data = NULL;
}

/*
 * Set a generic tag resolver.
 */

YAML_DECLARE(void)
yaml_parser_set_resolver(yaml_parser_t *parser,
        yaml_resolver_t *resolver, void *data)
{
    assert(parser);     /* Non-NULL parser object expected. */
    assert(!parser->resolver);  /* You can set the tag resolver only once. */
    assert(resolver);   /* Non-NULL resolver is expected. */

    parser->resolver = resolver;
    parser->resolver_data = data;
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

/*****************************************************************************
 * Parser API
 *****************************************************************************/

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
        yaml_event_destroy(&DEQUEUE(emitter, emitter->events));
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
 * Reset an emitter object.
 */

YAML_DECLARE(void)
yaml_emitter_reset(yaml_emitter_t *emitter)
{
    yaml_emitter_t copy = *emitter;

    assert(emitter);    /* Non-NULL emitter object expected. */

    while (!QUEUE_EMPTY(emitter, emitter->events)) {
        yaml_event_destroy(&DEQUEUE(emitter, emitter->events));
    }
    while (!STACK_EMPTY(empty, emitter->tag_directives)) {
        yaml_tag_directive_t tag_directive = POP(emitter, emitter->tag_directives);
        yaml_free(tag_directive.handle);
        yaml_free(tag_directive.prefix);
    }

    memset(emitter, 0, sizeof(yaml_emitter_t));

    IOSTRING_SET(emitter, emitter->output,
            copy.output.buffer, copy.output.capacity);
    IOSTRING_SET(emitter, emitter->raw_output,
            copy.raw_output.buffer, copy.raw_output.capacity);
    STACK_SET(emitter, emitter->states,
            copy.states.list, copy.states.capacity);
    QUEUE_SET(emitter, emitter->events,
            copy.events.list, copy.events.capacity);
    STACK_SET(emitter, emitter->indents,
            copy.indents.list, copy.indents.capacity);
    STACK_SET(emitter, emitter->tag_directives,
            copy.tag_directives.list, copy.tag_directives.capacity);
}

/*
 * Get the current emitter error.
 */

YAML_DECLARE(yaml_error_t *)
yaml_emitter_get_error(yaml_emitter_t *emitter)
{
    assert(emitter);    /* Non-NULL emitter object expected. */

    return &(emitter->error);
}

/*
 * Set a string output.
 */

YAML_DECLARE(void)
yaml_emitter_set_string_writer(yaml_emitter_t *emitter,
        unsigned char *buffer, size_t capacity, size_t *length)
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
 * Set a standard tag resolver.
 */

YAML_DECLARE(void)
yaml_emitter_set_standard_resolver(yaml_emitter_t *emitter)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->resolver); /* You can set the tag resolver only once. */

    emitter->resolver = yaml_standard_resolver;
    emitter->resolver_data = NULL;
}

/*
 * Set a generic tag resolver.
 */

YAML_DECLARE(void)
yaml_emitter_set_resolver(yaml_emitter_t *emitter,
        yaml_resolver_t *resolver, void *data)
{
    assert(emitter);    /* Non-NULL emitter object expected. */
    assert(!emitter->resolver); /* You can set the tag resolver only once. */
    assert(resolver);   /* Non-NULL resolver is expected. */

    emitter->resolver = resolver;
    emitter->resolver_data = data;
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

