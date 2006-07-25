
#include "yaml_private.h"

/*
 * API functions.
 */

YAML_DECLARE(int)
yaml_emitter_emit(yaml_emitter_t *emitter, yaml_event_t *event);

YAML_DECLARE(int)
yaml_emitter_emit_stream_start(yaml_emitter_t *emitter,
        yaml_encoding_t encoding);

YAML_DECLARE(int)
yaml_emitter_emit_stream_end(yaml_emitter_t *emitter);

YAML_DECLARE(int)
yaml_emitter_emit_document_start(yaml_emitter_t *emitter,
        yaml_version_directive_t *version_directive,
        yaml_tag_directive_t *tag_directives_start,
        yaml_tag_directive_t *tag_directives_end,
        int implicit);

YAML_DECLARE(int)
yaml_emitter_emit_document_end(yaml_emitter_t *emitter, int implicit);

YAML_DECLARE(int)
yaml_emitter_emit_alias(yaml_emitter_t *emitter, yaml_char_t *anchor);

YAML_DECLARE(int)
yaml_emitter_emit_scalar(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag,
        yaml_char_t *value, size_t length,
        int plain_implicit, int quoted_implicit,
        yaml_scalar_style_t style);

YAML_DECLARE(int)
yaml_emitter_emit_sequence_start(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag, int implicit,
        yaml_sequence_style_t style);

YAML_DECLARE(int)
yaml_emitter_emit_sequence_end(yaml_emitter_t *emitter);

YAML_DECLARE(int)
yaml_emitter_emit_mapping_start(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag, int implicit,
        yaml_mapping_style_t style);

YAML_DECLARE(int)
yaml_emitter_emit_mapping_end(yaml_emitter_t *emitter);

/*
 * Emit STREAM-START.
 */

YAML_DECLARE(int)
yaml_emitter_emit_stream_start(yaml_emitter_t *emitter,
        yaml_encoding_t encoding)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };

    assert(emitter);    /* Non-NULL emitter object is expected. */

    STREAM_START_EVENT_INIT(event, encoding, mark, mark);

    return yaml_emitter_emit(emitter, &event);
}

/*
 * Emit STREAM-END.
 */

YAML_DECLARE(int)
yaml_emitter_emit_stream_end(yaml_emitter_t *emitter)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };

    assert(emitter);    /* Non-NULL emitter object is expected. */

    STREAM_END_EVENT_INIT(event, mark, mark);

    return yaml_emitter_emit(emitter, &event);
}

/*
 * Emit DOCUMENT-START.
 */

YAML_DECLARE(int)
yaml_emitter_emit_document_start(yaml_emitter_t *emitter,
        yaml_version_directive_t *version_directive,
        yaml_tag_directive_t *tag_directives_start,
        yaml_tag_directive_t *tag_directives_end,
        int implicit)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_version_directive_t *version_directive_copy = NULL;
    struct {
        yaml_tag_directive_t *start;
        yaml_tag_directive_t *end;
        yaml_tag_directive_t *top;
    } tag_directives_copy = { NULL, NULL, NULL };
    yaml_tag_directive_t value = { NULL, NULL };

    assert(emitter);        /* Non-NULL emitter object is expected. */
    assert((tag_directives_start && tag_directives_end) ||
            (tag_directives_start == tag_directives_end));
                            /* Valid tag directives are expected. */

    if (version_directive) {
        version_directive_copy = yaml_malloc(sizeof(yaml_version_directive_t));
        if (!version_directive_copy) {
            emitter->error = YAML_MEMORY_ERROR;
            goto error;
        }
        version_directive_copy->major = version_directive->major;
        version_directive_copy->minor = version_directive->minor;
    }

    if (tag_directives_start != tag_directives_end) {
        yaml_tag_directive_t *tag_directive;
        if (!STACK_INIT(emitter, tag_directives_copy, INITIAL_STACK_SIZE))
            goto error;
        for (tag_directive = tag_directives_start;
                tag_directive != tag_directives_end; tag_directive ++) {
            value.handle = yaml_strdup(tag_directive->handle);
            value.prefix = yaml_strdup(tag_directive->prefix);
            if (!value.handle || !value.prefix) {
                emitter->error = YAML_MEMORY_ERROR;
                goto error;
            }
            if (!PUSH(emitter, tag_directives_copy, value))
                goto error;
            value.handle = NULL;
            value.prefix = NULL;
        }
    }

    DOCUMENT_START_EVENT_INIT(event, version_directive_copy,
            tag_directives_copy.start, tag_directives_copy.end,
            implicit, mark, mark);

    if (yaml_emitter_emit(emitter, &event)) {
        return 1;
    }

error:
    yaml_free(version_directive_copy);
    while (!STACK_EMPTY(emitter, tag_directives_copy)) {
        yaml_tag_directive_t value = POP(emitter, tag_directives_copy);
        yaml_free(value.handle);
        yaml_free(value.prefix);
    }
    STACK_DEL(emitter, tag_directives_copy);
    yaml_free(value.handle);
    yaml_free(value.prefix);

    return 0;
}

/*
 * Emit DOCUMENT-END.
 */

YAML_DECLARE(int)
yaml_emitter_emit_document_end(yaml_emitter_t *emitter, int implicit)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };

    assert(emitter);    /* Non-NULL emitter object is expected. */

    DOCUMENT_END_EVENT_INIT(event, implicit, mark, mark);

    return yaml_emitter_emit(emitter, &event);
}

/*
 * Emit ALIAS.
 */

YAML_DECLARE(int)
yaml_emitter_emit_alias(yaml_emitter_t *emitter, yaml_char_t *anchor)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;

    assert(emitter);    /* Non-NULL emitter object is expected. */
    assert(anchor);     /* Non-NULL anchor is expected. */

    anchor_copy = yaml_strdup(anchor);
    if (!anchor_copy) {
        emitter->error = YAML_MEMORY_ERROR;
        return 0;
    }

    ALIAS_EVENT_INIT(event, anchor_copy, mark, mark);

    if (yaml_emitter_emit(emitter, &event)) {
        return 1;
    }

    yaml_free(anchor_copy);

    return 0;
}

/*
 * Emit SCALAR.
 */

YAML_DECLARE(int)
yaml_emitter_emit_scalar(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag,
        yaml_char_t *value, size_t length,
        int plain_implicit, int quoted_implicit,
        yaml_scalar_style_t style)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;
    yaml_char_t *value_copy = NULL;

    assert(emitter);    /* Non-NULL emitter object is expected. */
    assert(value);      /* Non-NULL anchor is expected. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy) {
            emitter->error = YAML_MEMORY_ERROR;
            goto error;
        }
    }

    if (tag) {
        tag_copy = yaml_strdup(tag);
        if (!tag_copy) {
            emitter->error = YAML_MEMORY_ERROR;
            goto error;
        }
    }

    value_copy = yaml_malloc(length+1);
    if (!value_copy) {
        emitter->error = YAML_MEMORY_ERROR;
        goto error;
    }
    memcpy(value_copy, value, length);
    value_copy[length] = '\0';

    SCALAR_EVENT_INIT(event, anchor_copy, tag_copy, value_copy, length,
            plain_implicit, quoted_implicit, style, mark, mark);

    if (yaml_emitter_emit(emitter, &event)) {
        return 1;
    }

error:
    yaml_free(anchor_copy);
    yaml_free(tag_copy);
    yaml_free(value_copy);

    return 0;
}

/*
 * Emit SEQUENCE-START.
 */

YAML_DECLARE(int)
yaml_emitter_emit_sequence_start(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag, int implicit,
        yaml_sequence_style_t style)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;

    assert(emitter);    /* Non-NULL emitter object is expected. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy) {
            emitter->error = YAML_MEMORY_ERROR;
            goto error;
        }
    }

    if (tag) {
        tag_copy = yaml_strdup(tag);
        if (!tag_copy) {
            emitter->error = YAML_MEMORY_ERROR;
            goto error;
        }
    }

    SEQUENCE_START_EVENT_INIT(event, anchor_copy, tag_copy,
            implicit, style, mark, mark);

    if (yaml_emitter_emit(emitter, &event)) {
        return 1;
    }

error:
    yaml_free(anchor_copy);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Emit SEQUENCE-END.
 */

YAML_DECLARE(int)
yaml_emitter_emit_sequence_end(yaml_emitter_t *emitter)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };

    assert(emitter);    /* Non-NULL emitter object is expected. */

    SEQUENCE_END_EVENT_INIT(event, mark, mark);

    return yaml_emitter_emit(emitter, &event);
}

/*
 * Emit MAPPING-START.
 */

YAML_DECLARE(int)
yaml_emitter_emit_mapping_start(yaml_emitter_t *emitter,
        yaml_char_t *anchor, yaml_char_t *tag, int implicit,
        yaml_mapping_style_t style)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };
    yaml_char_t *anchor_copy = NULL;
    yaml_char_t *tag_copy = NULL;

    assert(emitter);    /* Non-NULL emitter object is expected. */

    if (anchor) {
        anchor_copy = yaml_strdup(anchor);
        if (!anchor_copy) {
            emitter->error = YAML_MEMORY_ERROR;
            goto error;
        }
    }

    if (tag) {
        tag_copy = yaml_strdup(tag);
        if (!tag_copy) {
            emitter->error = YAML_MEMORY_ERROR;
            goto error;
        }
    }

    MAPPING_START_EVENT_INIT(event, anchor_copy, tag_copy,
            implicit, style, mark, mark);

    if (yaml_emitter_emit(emitter, &event)) {
        return 1;
    }

error:
    yaml_free(anchor_copy);
    yaml_free(tag_copy);

    return 0;
}

/*
 * Emit MAPPING-END.
 */

YAML_DECLARE(int)
yaml_emitter_emit_mapping_end(yaml_emitter_t *emitter)
{
    yaml_event_t event;
    yaml_mark_t mark = { 0, 0, 0 };

    assert(emitter);    /* Non-NULL emitter object is expected. */

    MAPPING_END_EVENT_INIT(event, mark, mark);

    return yaml_emitter_emit(emitter, &event);
}

/*
 * Emit an event.
 */

YAML_DECLARE(int)
yaml_emitter_emit(yaml_emitter_t *emitter, yaml_event_t *event)
{
    return 0;
}

