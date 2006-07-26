
#include "yaml_private.h"

/*
 * API functions.
 */

YAML_DECLARE(int)
yaml_emitter_emit(yaml_emitter_t *emitter, yaml_event_t *event);

/*
 * Utility functions.
 */

static int
yaml_emitter_set_emitter_error(yaml_emitter_t *emitter, const char *problem);

static int
yaml_emitter_need_more_events(yaml_emitter_t *emitter);

static int
yaml_emitter_append_tag_directive(yaml_emitter_t *emitter,
        yaml_tag_directive_t value, int allow_duplicates);

static int
yaml_emitter_increase_indent(yaml_emitter_t *emitter,
        int flow, int indentless);

/*
 * State functions.
 */

static int
yaml_emitter_state_machine(yaml_emitter_t *emitter, yaml_event_t *event);

static int
yaml_emitter_emit_stream_start(yaml_emitter_t *emitter,
        yaml_event_t *event);

static int
yaml_emitter_emit_document_start(yaml_emitter_t *emitter,
        yaml_event_t *event, int first);

static int
yaml_emitter_emit_document_content(yaml_emitter_t *emitter,
        yaml_event_t *event);

static int
yaml_emitter_emit_document_end(yaml_emitter_t *emitter,
        yaml_event_t *event);

static int
yaml_emitter_emit_flow_sequence_item(yaml_emitter_t *emitter,
        yaml_event_t *event, int first);

static int
yaml_emitter_emit_flow_mapping_key(yaml_emitter_t *emitter,
        yaml_event_t *event, int first);

static int
yaml_emitter_emit_flow_mapping_value(yaml_emitter_t *emitter,
        yaml_event_t *event, int simple);

static int
yaml_emitter_emit_block_sequence_item(yaml_emitter_t *emitter,
        yaml_event_t *event, int first);

static int
yaml_emitter_emit_block_mapping_key(yaml_emitter_t *emitter,
        yaml_event_t *event, int first);

static int
yaml_emitter_emit_block_mapping_value(yaml_emitter_t *emitter,
        yaml_event_t *event, int simple);

static int
yaml_emitter_emit_node(yaml_emitter_t *emitter, yaml_event_t *event,
        int root, int sequence, int mapping, int simple_key);

static int
yaml_emitter_emit_alias(yaml_emitter_t *emitter, yaml_event_t *event);

static int
yaml_emitter_emit_scalar(yaml_emitter_t *emitter, yaml_event_t *event);

static int
yaml_emitter_emit_sequence_start(yaml_emitter_t *emitter, yaml_event_t *event);

static int
yaml_emitter_emit_mapping_start(yaml_emitter_t *emitter, yaml_event_t *event);

/*
 * Checkers.
 */

static int
yaml_emitter_check_empty_document(yaml_emitter_t *emitter);

static int
yaml_emitter_check_empty_sequence(yaml_emitter_t *emitter);

static int
yaml_emitter_check_empty_mapping(yaml_emitter_t *emitter);

static int
yaml_emitter_check_simple_key(yaml_emitter_t *emitter);

/*
 * Processors.
 */

static int
yaml_emitter_process_anchor(yaml_emitter_t *emitter,
        yaml_char_t *anchor, int alias);

static int
yaml_emitter_process_tag(yaml_emitter_t *emitter,
        yaml_char_t *tag);

static int
yaml_emitter_process_scalar(yaml_emitter_t *emitter,
        yaml_char_t *value, size_t length,
        int plain_implicit, int quoted_implicit,
        yaml_scalar_style_t style);

/*
 * Writers.
 */

static int
yaml_emitter_write_bom(yaml_emitter_t *emitter);

static int
yaml_emitter_write_version_directive(yaml_emitter_t *emitter,
        yaml_version_directive_t version_directive);

static int
yaml_emitter_write_tag_directive(yaml_emitter_t *emitter,
        yaml_tag_directive_t tag_directive);

static int
yaml_emitter_write_indent(yaml_emitter_t *emitter);

static int
yaml_emitter_write_indicator(yaml_emitter_t *emitter,
        char *indicator, int need_whitespace,
        int is_whitespace, int is_indention);

/*
 * Set an emitter error and return 0.
 */

static int
yaml_emitter_set_emitter_error(yaml_emitter_t *emitter, const char *problem)
{
    emitter->error = YAML_EMITTER_ERROR;
    emitter->problem = problem;

    return 0;
}

/*
 * Emit an event.
 */

YAML_DECLARE(int)
yaml_emitter_emit(yaml_emitter_t *emitter, yaml_event_t *event)
{
    if (!ENQUEUE(emitter, emitter->events, *event)) {
        yaml_event_delete(event);
        return 0;
    }

    while (!yaml_emitter_need_more_events(emitter)) {
        if (!yaml_emitter_state_machine(emitter, emitter->events.head)) {
            return 0;
        }
        DEQUEUE(emitter, emitter->events);
    }

    return 1;
}

/*
 * Check if we need to accumulate more events before emitting.
 *
 * We accumulate extra
 *  - 1 event for DOCUMENT-START
 *  - 2 events for SEQUENCE-START
 *  - 3 events for MAPPING-START
 */

static int
yaml_emitter_need_more_events(yaml_emitter_t *emitter)
{
    int level = 0;
    int accumulate = 0;
    yaml_event_t *event;

    if (QUEUE_EMPTY(emitter, emitter->events))
        return 1;

    switch (emitter->events.head->type) {
        case YAML_DOCUMENT_START_EVENT:
            accumulate = 1;
            break;
        case YAML_SEQUENCE_START_EVENT:
            accumulate = 2;
            break;
        case YAML_MAPPING_START_EVENT:
            accumulate = 3;
            break;
        default:
            return 0;
    }

    if (emitter->events.tail - emitter->events.head > accumulate)
        return 0;

    for (event = emitter->events.head; event != emitter->events.tail; event ++) {
        switch (event->type) {
            case YAML_STREAM_START_EVENT:
            case YAML_DOCUMENT_START_EVENT:
            case YAML_SEQUENCE_START_EVENT:
            case YAML_MAPPING_START_EVENT:
                level += 1;
                break;
            case YAML_STREAM_END_EVENT:
            case YAML_DOCUMENT_END_EVENT:
            case YAML_SEQUENCE_END_EVENT:
            case YAML_MAPPING_END_EVENT:
                level -= 1;
                break;
            default:
                break;
        }
        if (!level)
            return 0;
    }

    return 1;
}

/*
 * Append a directive to the directives stack.
 */

static int
yaml_emitter_append_tag_directive(yaml_emitter_t *emitter,
        yaml_tag_directive_t value, int allow_duplicates)
{
    yaml_tag_directive_t *tag_directive;
    yaml_tag_directive_t copy = { NULL, NULL };

    for (tag_directive = emitter->tag_directives.start;
            tag_directive != emitter->tag_directives.top; tag_directive ++) {
        if (strcmp((char *)value.handle, (char *)tag_directive->handle) == 0) {
            if (allow_duplicates)
                return 1;
            return yaml_emitter_set_emitter_error(emitter,
                    "duplicate %TAG directive");
        }
    }

    copy.handle = yaml_strdup(value.handle);
    copy.prefix = yaml_strdup(value.prefix);
    if (!copy.handle || !copy.prefix) {
        emitter->error = YAML_MEMORY_ERROR;
        goto error;
    }

    if (!PUSH(emitter, emitter->tag_directives, copy))
        goto error;

    return 1;

error:
    yaml_free(copy.handle);
    yaml_free(copy.prefix);
    return 0;
}

/*
 * Increase the indentation level.
 */

static int
yaml_emitter_increase_indent(yaml_emitter_t *emitter,
        int flow, int indentless)
{
    if (!PUSH(emitter, emitter->indents, emitter->indent))
        return 0;

    if (emitter->indent < 0) {
        emitter->indent = flow ? emitter->best_indent : 0;
    }
    else if (!indentless) {
        emitter->indent += emitter->best_indent;
    }

    return 1;
}

/*
 * State dispatcher.
 */

static int
yaml_emitter_state_machine(yaml_emitter_t *emitter, yaml_event_t *event)
{
    switch (emitter->state)
    {
        case YAML_EMIT_STREAM_START_STATE:
            return yaml_emitter_emit_stream_start(emitter, event);

        case YAML_EMIT_FIRST_DOCUMENT_START_STATE:
            return yaml_emitter_emit_document_start(emitter, event, 1);

        case YAML_EMIT_DOCUMENT_START_STATE:
            return yaml_emitter_emit_document_start(emitter, event, 0);

        case YAML_EMIT_DOCUMENT_CONTENT_STATE:
            return yaml_emitter_emit_document_content(emitter, event);

        case YAML_EMIT_DOCUMENT_END_STATE:
            return yaml_emitter_emit_document_end(emitter, event);

        case YAML_EMIT_FLOW_SEQUENCE_FIRST_ITEM_STATE:
            return yaml_emitter_emit_flow_sequence_item(emitter, event, 1);

        case YAML_EMIT_FLOW_SEQUENCE_ITEM_STATE:
            return yaml_emitter_emit_flow_sequence_item(emitter, event, 0);

        case YAML_EMIT_FLOW_MAPPING_FIRST_KEY_STATE:
            return yaml_emitter_emit_flow_mapping_key(emitter, event, 1);

        case YAML_EMIT_FLOW_MAPPING_KEY_STATE:
            return yaml_emitter_emit_flow_mapping_key(emitter, event, 0);

        case YAML_EMIT_FLOW_MAPPING_SIMPLE_VALUE_STATE:
            return yaml_emitter_emit_flow_mapping_value(emitter, event, 1);

        case YAML_EMIT_FLOW_MAPPING_VALUE_STATE:
            return yaml_emitter_emit_flow_mapping_value(emitter, event, 0);

        case YAML_EMIT_BLOCK_SEQUENCE_FIRST_ITEM_STATE:
            return yaml_emitter_emit_block_sequence_item(emitter, event, 1);

        case YAML_EMIT_BLOCK_SEQUENCE_ITEM_STATE:
            return yaml_emitter_emit_block_sequence_item(emitter, event, 0);

        case YAML_EMIT_BLOCK_MAPPING_FIRST_KEY_STATE:
            return yaml_emitter_emit_block_mapping_key(emitter, event, 1);

        case YAML_EMIT_BLOCK_MAPPING_KEY_STATE:
            return yaml_emitter_emit_block_mapping_key(emitter, event, 0);

        case YAML_EMIT_BLOCK_MAPPING_SIMPLE_VALUE_STATE:
            return yaml_emitter_emit_block_mapping_value(emitter, event, 1);

        case YAML_EMIT_BLOCK_MAPPING_VALUE_STATE:
            return yaml_emitter_emit_block_mapping_value(emitter, event, 0);

        case YAML_EMIT_END_STATE:
            return yaml_emitter_set_emitter_error(emitter,
                    "expected nothing after STREAM-END");

        default:
            assert(1);      /* Invalid state. */
    }

    return 0;
}

/*
 * Expect STREAM-START.
 */

static int
yaml_emitter_emit_stream_start(yaml_emitter_t *emitter,
        yaml_event_t *event)
{
    if (event->type == YAML_STREAM_START_EVENT)
    {
        if (!emitter->encoding) {
            emitter->encoding = event->data.stream_start.encoding;
        }

        if (!emitter->encoding) {
            emitter->encoding = YAML_UTF8_ENCODING;
        }

        if (emitter->best_indent < 2 || emitter->best_indent > 9) {
            emitter->best_indent  = 2;
        }

        if (emitter->best_width >= 0
                && emitter->best_width <= emitter->best_indent*2) {
            emitter->best_width = 80;
        }

        if (emitter->best_width < 0) {
            emitter->best_width = INT_MAX;
        }
        
        if (!emitter->line_break) {
            emitter->line_break = YAML_LN_BREAK;
        }

        emitter->indent = -1;

        emitter->line = 0;
        emitter->column = 0;
        emitter->whitespace = 1;
        emitter->indention = 1;

        if (emitter->encoding != YAML_UTF8_ENCODING) {
            if (!yaml_emitter_write_bom(emitter))
                return 0;
        }

        emitter->state = YAML_EMIT_FIRST_DOCUMENT_START_STATE;

        return 1;
    }

    return yaml_emitter_set_emitter_error(emitter,
            "expected STREAM-START");
}

static int
yaml_emitter_emit_document_start(yaml_emitter_t *emitter,
        yaml_event_t *event, int first)
{
    if (event->type == YAML_DOCUMENT_START_EVENT)
    {
        yaml_tag_directive_t default_tag_directives[] = {
            {(yaml_char_t *)"!", (yaml_char_t *)"!"},
            {(yaml_char_t *)"!!", (yaml_char_t *)"tag:yaml.org,2002:"},
            {NULL, NULL}
        };
        yaml_tag_directive_t *tag_directive;
        int implicit;

        if (event->data.document_start.version_directive) {
            if (event->data.document_start.version_directive->major != 1
                    || event->data.document_start.version_directive-> minor != 1) {
                return yaml_emitter_set_emitter_error(emitter,
                        "incompatible %YAML directive");
            }
        }

        for (tag_directive = event->data.document_start.tag_directives.start;
                tag_directive != event->data.document_start.tag_directives.end;
                tag_directive ++) {
            if (!yaml_emitter_append_tag_directive(emitter, *tag_directive, 0))
                return 0;
        }

        for (tag_directive = default_tag_directives;
                tag_directive->handle; tag_directive ++) {
            if (!yaml_emitter_append_tag_directive(emitter, *tag_directive, 1))
                return 0;
        }

        implicit = event->data.document_start.implicit;
        if (!first || emitter->canonical) {
            implicit = 0;
        }

        if (event->data.document_start.version_directive) {
            implicit = 0;
            if (!yaml_emitter_write_version_directive(emitter,
                        *event->data.document_start.version_directive))
                return 0;
        }
        
        if (event->data.document_start.tag_directives.start
                != event->data.document_start.tag_directives.end) {
            implicit = 0;
            for (tag_directive = event->data.document_start.tag_directives.start;
                    tag_directive != event->data.document_start.tag_directives.end;
                    tag_directive ++) {
                if (!yaml_emitter_write_tag_directive(emitter, *tag_directive))
                    return 0;
            }
        }

        if (yaml_emitter_check_empty_document(emitter)) {
            implicit = 0;
        }

        if (!implicit) {
            if (!yaml_emitter_write_indent(emitter))
                return 0;
            if (!yaml_emitter_write_indicator(emitter, "---", 1, 0, 0))
                return 0;
            if (emitter->canonical) {
                if (!yaml_emitter_write_indent(emitter))
                    return 0;
            }
        }

        emitter->state = YAML_EMIT_DOCUMENT_CONTENT_STATE;

        return 1;
    }

    else if (event->type == YAML_STREAM_END_EVENT)
    {
        if (!yaml_emitter_flush(emitter))
            return 0;

        emitter->state = YAML_EMIT_END_STATE;

        return 1;
    }

    return yaml_emitter_set_emitter_error(emitter,
            "expected DOCUMENT-START or STREAM-END");
}

static int
yaml_emitter_emit_document_content(yaml_emitter_t *emitter,
        yaml_event_t *event)
{
    if (!PUSH(emitter, emitter->states, YAML_EMIT_DOCUMENT_END_STATE))
        return 0;

    return yaml_emitter_emit_node(emitter, event, 1, 0, 0, 0);
}

static int
yaml_emitter_emit_document_end(yaml_emitter_t *emitter,
        yaml_event_t *event)
{
    if (event->type == YAML_DOCUMENT_END_EVENT)
    {
        if (!yaml_emitter_write_indent(emitter))
            return 0;
        if (!event->data.document_end.implicit) {
            if (!yaml_emitter_write_indicator(emitter, "...", 1, 0, 0))
                return 0;
            if (!yaml_emitter_write_indent(emitter))
                return 0;
        }
        if (!yaml_emitter_flush(emitter))
            return 0;

        emitter->state = YAML_EMIT_DOCUMENT_START_STATE;

        return 1;
    }

    return yaml_emitter_set_emitter_error(emitter,
            "expected DOCUMENT-END");
}

static int
yaml_emitter_emit_flow_sequence_item(yaml_emitter_t *emitter,
        yaml_event_t *event, int first)
{
    if (first)
    {
        if (!yaml_emitter_write_indicator(emitter, "[", 1, 1, 0))
            return 0;
        if (!yaml_emitter_increase_indent(emitter, 1, 0))
            return 0;
        emitter->flow_level ++;
    }

    if (event->type == YAML_SEQUENCE_END_EVENT)
    {
        emitter->flow_level --;
        emitter->indent = POP(emitter, emitter->indents);
        if (emitter->canonical && !first) {
            if (!yaml_emitter_write_indicator(emitter, ",", 0, 0, 0))
                return 0;
            if (!yaml_emitter_write_indent(emitter))
                return 0;
        }
        if (!yaml_emitter_write_indicator(emitter, "]", 0, 0, 0))
            return 0;
        emitter->state = POP(emitter, emitter->states);

        return 1;
    }

    if (emitter->canonical || emitter->column > emitter->best_width) {
        if (!yaml_emitter_write_indent(emitter))
            return 0;
    }
    if (PUSH(emitter, emitter->states, YAML_EMIT_FLOW_SEQUENCE_ITEM_STATE))
        return 0;

    return yaml_emitter_emit_node(emitter, event, 0, 1, 0, 0);
}

static int
yaml_emitter_emit_flow_mapping_key(yaml_emitter_t *emitter,
        yaml_event_t *event, int first)
{
    if (first)
    {
        if (!yaml_emitter_write_indicator(emitter, "{", 1, 1, 0))
            return 0;
        if (!yaml_emitter_increase_indent(emitter, 1, 0))
            return 0;
        emitter->flow_level ++;
    }

    if (event->type == YAML_MAPPING_END_EVENT)
    {
        emitter->flow_level --;
        emitter->indent = POP(emitter, emitter->indents);
        if (emitter->canonical && !first) {
            if (!yaml_emitter_write_indicator(emitter, ",", 0, 0, 0))
                return 0;
            if (!yaml_emitter_write_indent(emitter))
                return 0;
        }
        if (!yaml_emitter_write_indicator(emitter, "}", 0, 0, 0))
            return 0;
        emitter->state = POP(emitter, emitter->states);

        return 1;
    }

    if (!first) {
        if (!yaml_emitter_write_indicator(emitter, ",", 0, 0, 0))
            return 0;
    }
    if (emitter->canonical || emitter->column > emitter->best_width) {
        if (!yaml_emitter_write_indent(emitter))
            return 0;
    }

    if (!emitter->canonical && yaml_emitter_check_simple_key(emitter))
    {
        if (!PUSH(emitter, emitter->states,
                    YAML_EMIT_FLOW_MAPPING_SIMPLE_VALUE_STATE))
            return 0;

        return yaml_emitter_emit_node(emitter, event, 0, 0, 1, 1);
    }
    else
    {
        if (!yaml_emitter_write_indicator(emitter, "?", 1, 0, 0))
            return 0;
        if (!PUSH(emitter, emitter->states,
                    YAML_EMIT_FLOW_MAPPING_VALUE_STATE))
            return 0;

        return yaml_emitter_emit_node(emitter, event, 0, 0, 1, 0);
    }
}

static int
yaml_emitter_emit_flow_mapping_value(yaml_emitter_t *emitter,
        yaml_event_t *event, int simple)
{
    if (simple) {
        if (!yaml_emitter_write_indicator(emitter, ":", 0, 0, 0))
            return 0;
    }
    else {
        if (emitter->canonical || emitter->column > emitter->best_width) {
            if (!yaml_emitter_write_indent(emitter))
                return 0;
        }
        if (!yaml_emitter_write_indicator(emitter, ":", 1, 0, 0))
            return 0;
    }
    if (!PUSH(emitter, emitter->states, YAML_EMIT_FLOW_MAPPING_KEY_STATE))
        return 0;
    return yaml_emitter_emit_node(emitter, event, 0, 0, 1, 0);
}

static int
yaml_emitter_emit_block_sequence_item(yaml_emitter_t *emitter,
        yaml_event_t *event, int first)
{
    if (first)
    {
        if (!yaml_emitter_increase_indent(emitter, 0,
                    (emitter->mapping_context && !emitter->indention)))
            return 0;
    }

    if (event->type == YAML_SEQUENCE_END_EVENT)
    {
        emitter->indent = POP(emitter, emitter->indents);
        emitter->state = POP(emitter, emitter->states);

        return 1;
    }

    if (!yaml_emitter_write_indent(emitter))
        return 0;
    if (!yaml_emitter_write_indicator(emitter, "-", 1, 0, 1))
        return 0;
    if (!PUSH(emitter, emitter->states,
                YAML_EMIT_BLOCK_SEQUENCE_ITEM_STATE))
        return 0;

    return yaml_emitter_emit_node(emitter, event, 0, 1, 0, 0);
}

static int
yaml_emitter_emit_block_mapping_key(yaml_emitter_t *emitter,
        yaml_event_t *event, int first)
{
    if (first)
    {
        if (!yaml_emitter_increase_indent(emitter, 0, 0))
            return 0;
    }

    if (event->type == YAML_MAPPING_END_EVENT)
    {
        emitter->indent = POP(emitter, emitter->indents);
        emitter->state = POP(emitter, emitter->states);

        return 1;
    }

    if (!yaml_emitter_write_indent(emitter))
        return 0;

    if (yaml_emitter_check_simple_key(emitter))
    {
        if (!PUSH(emitter, emitter->states,
                    YAML_EMIT_BLOCK_MAPPING_SIMPLE_VALUE_STATE))
            return 0;

        return yaml_emitter_emit_node(emitter, event, 0, 0, 1, 1);
    }
    else
    {
        if (!yaml_emitter_write_indicator(emitter, "?", 1, 0, 1))
            return 0;
        if (!PUSH(emitter, emitter->states,
                    YAML_EMIT_BLOCK_MAPPING_VALUE_STATE))
            return 0;

        return yaml_emitter_emit_node(emitter, event, 0, 0, 1, 0);
    }
}

static int
yaml_emitter_emit_block_mapping_value(yaml_emitter_t *emitter,
        yaml_event_t *event, int simple)
{
    if (simple) {
        if (!yaml_emitter_write_indicator(emitter, ":", 0, 0, 0))
            return 0;
    }
    else {
        if (!yaml_emitter_write_indent(emitter))
            return 0;
        if (!yaml_emitter_write_indicator(emitter, ":", 1, 0, 1))
            return 0;
    }
    if (!PUSH(emitter, emitter->states,
                YAML_EMIT_BLOCK_MAPPING_KEY_STATE))
        return 0;

    return yaml_emitter_emit_node(emitter, event, 0, 0, 1, 0);
}

static int
yaml_emitter_emit_node(yaml_emitter_t *emitter, yaml_event_t *event,
        int root, int sequence, int mapping, int simple_key)
{
    emitter->root_context = root;
    emitter->sequence_context = sequence;
    emitter->mapping_context = mapping;
    emitter->simple_key_context = simple_key;

    switch (event->type)
    {
        case YAML_ALIAS_EVENT:
            return yaml_emitter_emit_alias(emitter, event);

        case YAML_SCALAR_EVENT:
            return yaml_emitter_emit_scalar(emitter, event);

        case YAML_SEQUENCE_START_EVENT:
            return yaml_emitter_emit_sequence_start(emitter, event);

        case YAML_MAPPING_START_EVENT:
            return yaml_emitter_emit_mapping_start(emitter, event);

        default:
            return yaml_emitter_set_emitter_error(emitter,
                    "expected SCALAR, SEQUENCE-START, MAPPING-START, or ALIAS");
    }

    return 0;
}

static int
yaml_emitter_emit_alias(yaml_emitter_t *emitter, yaml_event_t *event)
{
    if (!yaml_emitter_process_anchor(emitter, event->data.alias.anchor, 1))
        return 0;
    emitter->state = POP(emitter, emitter->states);

    return 1;
}

static int
yaml_emitter_emit_scalar(yaml_emitter_t *emitter, yaml_event_t *event)
{
    if (!yaml_emitter_process_anchor(emitter, event->data.scalar.anchor, 0))
        return 0;
    if (!yaml_emitter_process_tag(emitter, event->data.scalar.tag))
        return 0;
    if (!yaml_emitter_increase_indent(emitter, 1, 0))
        return 0;
    if (!yaml_emitter_process_scalar(emitter,
                event->data.scalar.value, event->data.scalar.length,
                event->data.scalar.plain_implicit,
                event->data.scalar.quoted_implicit,
                event->data.scalar.style))
        return 0;
    emitter->indent = POP(emitter, emitter->indents);
    emitter->state = POP(emitter, emitter->states);

    return 1;
}

static int
yaml_emitter_emit_sequence_start(yaml_emitter_t *emitter, yaml_event_t *event)
{
    if (!yaml_emitter_process_anchor(emitter,
                event->data.sequence_start.anchor, 0))
        return 0;
    if (!yaml_emitter_process_tag(emitter,
                event->data.sequence_start.tag))
        return 0;

    if (emitter->flow_level || emitter->canonical
            || event->data.sequence_start.style == YAML_FLOW_SEQUENCE_STYLE
            || yaml_emitter_check_empty_sequence(emitter)) {
        emitter->state = YAML_EMIT_FLOW_SEQUENCE_FIRST_ITEM_STATE;
    }
    else {
        emitter->state = YAML_EMIT_BLOCK_SEQUENCE_FIRST_ITEM_STATE;
    }

    return 1;
}

static int
yaml_emitter_emit_mapping_start(yaml_emitter_t *emitter, yaml_event_t *event)
{
    if (!yaml_emitter_process_anchor(emitter,
                event->data.mapping_start.anchor, 0))
        return 0;
    if (!yaml_emitter_process_tag(emitter,
                event->data.mapping_start.tag))
        return 0;

    if (emitter->flow_level || emitter->canonical
            || event->data.mapping_start.style == YAML_FLOW_MAPPING_STYLE
            || yaml_emitter_check_empty_mapping(emitter)) {
        emitter->state = YAML_EMIT_FLOW_MAPPING_FIRST_KEY_STATE;
    }
    else {
        emitter->state = YAML_EMIT_BLOCK_MAPPING_FIRST_KEY_STATE;
    }

    return 1;
}

