
/*
 * The parser implements the following grammar:
 *
 * stream               ::= STREAM-START implicit_document? explicit_document* STREAM-END
 * implicit_document    ::= block_node DOCUMENT-END*
 * explicit_document    ::= DIRECTIVE* DOCUMENT-START block_node? DOCUMENT-END*
 * block_node_or_indentless_sequence    ::=
 *                          ALIAS
 *                          | properties (block_content | indentless_block_sequence)?
 *                          | block_content
 *                          | indentless_block_sequence
 * block_node           ::= ALIAS
 *                          | properties block_content?
 *                          | block_content
 * flow_node            ::= ALIAS
 *                          | properties flow_content?
 *                          | flow_content
 * properties           ::= TAG ANCHOR? | ANCHOR TAG?
 * block_content        ::= block_collection | flow_collection | SCALAR
 * flow_content         ::= flow_collection | SCALAR
 * block_collection     ::= block_sequence | block_mapping
 * flow_collection      ::= flow_sequence | flow_mapping
 * block_sequence       ::= BLOCK-SEQUENCE-START (BLOCK-ENTRY block_node?)* BLOCK-END
 * indentless_sequence  ::= (BLOCK-ENTRY block_node?)+
 * block_mapping        ::= BLOCK-MAPPING_START
 *                          ((KEY block_node_or_indentless_sequence?)?
 *                          (VALUE block_node_or_indentless_sequence?)?)*
 *                          BLOCK-END
 * flow_sequence        ::= FLOW-SEQUENCE-START
 *                          (flow_sequence_entry FLOW-ENTRY)*
 *                          flow_sequence_entry?
 *                          FLOW-SEQUENCE-END
 * flow_sequence_entry  ::= flow_node | KEY flow_node? (VALUE flow_node?)?
 * flow_mapping         ::= FLOW-MAPPING-START
 *                          (flow_mapping_entry FLOW-ENTRY)*
 *                          flow_mapping_entry?
 *                          FLOW-MAPPING-END
 * flow_mapping_entry   ::= flow_node | KEY flow_node? (VALUE flow_node?)?
 */

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml.h>

#include <assert.h>

/*
 * Public API declarations.
 */

YAML_DECLARE(yaml_event_t *)
yaml_parser_get_event(yaml_parser_t *parser);

YAML_DECLARE(yaml_event_t *)
yaml_parser_peek_event(yaml_parser_t *parser);

/*
 * Error handling.
 */

static int
yaml_parser_set_parser_error(yaml_parser_t *parser,
        const char *problem, yaml_mark_t problem_mark);

static int
yaml_parser_set_parser_error_context(yaml_parser_t *parser,
        const char *context, yaml_mark_t context_mark,
        const char *problem, yaml_mark_t problem_mark);

/*
 * Buffers and lists.
 */

static int
yaml_parser_resize_list(yaml_parser_t *parser, void **buffer, size_t *size,
        size_t item_size);

static int
yaml_parser_append_state(yaml_parser_t *parser, yaml_parser_state_t state);

static int
yaml_parser_append_mark(yaml_parser_t *parser, yaml_mark_t mark);

/*
 * State functions.
 */

static yaml_event_t *
yaml_parser_state_machine(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_stream_start(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_document_start(yaml_parser_t *parser, int implicit);

static yaml_event_t *
yaml_parser_parse_document_content(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_document_end(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_node(yaml_parser_t *parser,
        int block, int indentless_sequence);

static yaml_event_t *
yaml_parser_parse_block_sequence_entry(yaml_parser_t *parser, int first);

static yaml_event_t *
yaml_parser_parse_indentless_sequence_entry(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_block_mapping_key(yaml_parser_t *parser, int first);

static yaml_event_t *
yaml_parser_parse_block_mapping_value(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry(yaml_parser_t *parser, int first);

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry_mapping_key(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry_mapping_value(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry_mapping_end(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_flow_mapping_key(yaml_parser_t *parser, int first);

static yaml_event_t *
yaml_parser_parse_flow_mapping_value(yaml_parser_t *parser, int empty);

/*
 * Utility functions.
 */

static yaml_event_t *
yaml_parser_process_empty_scalar(yaml_parser_t *parser, yaml_mark_t mark);

static int
yaml_parser_process_directives(yaml_parser_t *parser);

/*
 * Get the next event and advance the parser.
 */

YAML_DECLARE(yaml_event_t *)
yaml_parser_get_event(yaml_parser_t *parser)
{
    yaml_event_t *value;

    /* Update the current event if needed. */
    
    if (!parser->current_event) {
        parser->current_event = yaml_parser_state_machine(parser);
    }

    /* Return and clear the current event. */

    value = parser->current_event;
    parser->current_event = NULL;
    return value;
}

/*
 * Peek the next event.
 */

YAML_DECLARE(yaml_event_t *)
yaml_parser_peek_event(yaml_parser_t *parser)
{
    yaml_event_t *value;

    /* Update the current event if needed. */
    
    if (!parser->current_event) {
        parser->current_event = yaml_parser_state_machine(parser);
    }

    /* Return the current event. */

    return parser->current_event;
}

/*
 * Set parser error.
 */

static int
yaml_parser_set_parser_error(yaml_parser_t *parser,
        const char *problem, yaml_mark_t problem_mark)
{
    parser->error = YAML_PARSER_ERROR;
    parser->problem = problem;
    parser->problem_mark = problem_mark;

    return 0;
}

static int
yaml_parser_set_parser_error_context(yaml_parser_t *parser,
        const char *context, yaml_mark_t context_mark,
        const char *problem, yaml_mark_t problem_mark)
{
    parser->error = YAML_PARSER_ERROR;
    parser->context = context;
    parser->context_mark = context_mark;
    parser->problem = problem;
    parser->problem_mark = problem_mark;

    return 0;
}

/*
 * Push a state to the state stack.
 */

static int
yaml_parser_append_state(yaml_parser_t *parser, yaml_parser_state_t state)
{
    if (parser->states_length == parser->states_size-1) {
        if (!yaml_parser_resize_list(parser, (void **)&parser->states,
                    &parser->states_size, sizeof(yaml_parser_state_t)))
            return 0;
    }

    parser->states[parser->states_length++] = state;
    return 1;
}

/*
 * Push a mark to the mark stack.
 */

static int
yaml_parser_append_mark(yaml_parser_t *parser, yaml_mark_t mark)
{
    if (parser->marks_length == parser->marks_size-1) {
        if (!yaml_parser_resize_list(parser, (void **)&parser->marks,
                    &parser->marks_size, sizeof(yaml_mark_t)))
            return 0;
    }

    parser->marks[parser->marks_length++] = mark;
    return 1;
}

/*
 * State dispatcher.
 */

static yaml_event_t *
yaml_parser_state_machine(yaml_parser_t *parser)
{
    assert (parser->state != YAML_PARSE_END_STATE);

    switch (parser->state)
    {
        case YAML_PARSE_STREAM_START_STATE:
            return yaml_parser_parse_stream_start(parser);

        case YAML_PARSE_IMPLICIT_DOCUMENT_START_STATE:
            return yaml_parser_parse_document_start(parser, 1);

        case YAML_PARSE_DOCUMENT_START_STATE:
            return yaml_parser_parse_document_start(parser, 0);

        case YAML_PARSE_DOCUMENT_CONTENT_STATE:
            return yaml_parser_parse_document_content(parser);

        case YAML_PARSE_DOCUMENT_END_STATE:
            return yaml_parser_parse_document_end(parser);

        case YAML_PARSE_BLOCK_NODE_STATE:
            return yaml_parser_parse_node(parser, 1, 0);

        case YAML_PARSE_BLOCK_NODE_OR_INDENTLESS_SEQUENCE_STATE:
            return yaml_parser_parse_node(parser, 1, 1);

        case YAML_PARSE_FLOW_NODE_STATE:
            return yaml_parser_parse_node(parser, 0, 0);

        case YAML_PARSE_BLOCK_SEQUENCE_FIRST_ENTRY_STATE:
            return yaml_parser_parse_block_sequence_entry(parser, 1);

        case YAML_PARSE_BLOCK_SEQUENCE_ENTRY_STATE:
            return yaml_parser_parse_block_sequence_entry(parser, 0);

        case YAML_PARSE_INDENTLESS_SEQUENCE_ENTRY_STATE:
            return yaml_parser_parse_indentless_sequence_entry(parser);

        case YAML_PARSE_BLOCK_MAPPING_FIRST_KEY_STATE:
            return yaml_parser_parse_block_mapping_key(parser, 1);

        case YAML_PARSE_BLOCK_MAPPING_KEY_STATE:
            return yaml_parser_parse_block_mapping_key(parser, 0);

        case YAML_PARSE_BLOCK_MAPPING_VALUE_STATE:
            return yaml_parser_parse_block_mapping_value(parser);

        case YAML_PARSE_FLOW_SEQUENCE_FIRST_ENTRY_STATE:
            return yaml_parser_parse_flow_sequence_entry(parser, 1);

        case YAML_PARSE_FLOW_SEQUENCE_ENTRY_STATE:
            return yaml_parser_parse_flow_sequence_entry(parser, 0);

        case YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_KEY_STATE:
            return yaml_parser_parse_flow_sequence_entry_mapping_key(parser);

        case YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_VALUE_STATE:
            return yaml_parser_parse_flow_sequence_entry_mapping_value(parser);

        case YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_END_STATE:
            return yaml_parser_parse_flow_sequence_entry_mapping_end(parser);

        case YAML_PARSE_FLOW_MAPPING_FIRST_KEY_STATE:
            return yaml_parser_parse_flow_mapping_key(parser, 1);

        case YAML_PARSE_FLOW_MAPPING_KEY_STATE:
            return yaml_parser_parse_flow_mapping_key(parser, 0);

        case YAML_PARSE_FLOW_MAPPING_VALUE_STATE:
            return yaml_parser_parse_flow_mapping_value(parser, 0);

        case YAML_PARSE_FLOW_MAPPING_EMPTY_VALUE_STATE:
            return yaml_parser_parse_flow_mapping_value(parser, 1);
    }
    assert(1);
}

/*
 * Parse the production:
 * stream   ::= STREAM-START implicit_document? explicit_document* STREAM-END
 *              ************
 */

static yaml_event_t *
yaml_parser_parse_stream_start(yaml_parser_t *parser)
{
    yaml_token_t *token;
    yaml_event_t *event;

    token = yaml_parser_get_token(parser);
    if (!token) return NULL;

    assert(token->type == YAML_STREAM_START_TOKEN);

    event = yaml_stream_start_event_new(token->data.stream_start.encoding,
            token->start_mark, token->start_mark);
    yaml_token_delete(token);
    if (!event) {
        parser->error = YAML_MEMORY_ERROR;
        return NULL;
    }

    parser->state = YAML_PARSE_IMPLICIT_DOCUMENT_START_STATE;

    return event;
}

/*
 * Parse the productions:
 * implicit_document    ::= block_node DOCUMENT-END*
 *                          *
 * explicit_document    ::= DIRECTIVE* DOCUMENT-START block_node? DOCUMENT-END*
 *                          *************************
 */

static yaml_event_t *
yaml_parser_parse_document_start(yaml_parser_t *parser, int implicit)
{
    yaml_token_t *token;
    yaml_event_t *event;

    token = yaml_parser_peek_token(parser);
    if (!token) return NULL;

    /* Parse an implicit document. */

    if (implicit && token->type != YAML_VERSION_DIRECTIVE_TOKEN &&
            token->type != YAML_TAG_DIRECTIVE_TOKEN &&
            token->type != YAML_DOCUMENT_START_TOKEN &&
            token->type != YAML_STREAM_END_TOKEN)
    {
        if (!yaml_parser_process_directives(parser)) return NULL;
        if (!yaml_parser_append_state(parser, YAML_PARSE_DOCUMENT_END_STATE))
            return NULL;
        parser->state = YAML_PARSE_BLOCK_NODE_STATE;
        event = yaml_document_start_event_new(
                parser->version_directive, parser->tag_directives, 1,
                token->start_mark, token->start_mark);
        if (!event) return NULL;
        return event;
    }

    /* Parse an explicit document. */

    else if (token->type != YAML_STREAM_END_TOKEN)
    {
        yaml_mark_t start_mark, end_mark;
        start_mark = token->start_mark;
        if (!yaml_parser_process_directives(parser)) return NULL;
        token = yaml_parser_peek_token(parser);
        if (!token) return NULL;
        if (token->type != YAML_DOCUMENT_START_TOKEN) {
            yaml_parser_set_parser_error(parser,
                    "did not found expected <document start>", token->start_mark);
            return NULL;
        }
        token = yaml_parser_get_token(parser);
        end_mark = token->end_mark;
        yaml_token_delete(token);
        if (!yaml_parser_append_state(parser, YAML_PARSE_DOCUMENT_END_STATE))
            return NULL;
        parser->state = YAML_PARSE_DOCUMENT_CONTENT_STATE;
        event = yaml_document_start_event_new(
                parser->version_directive, parser->tag_directives, 0,
                start_mark, end_mark);
        if (!event) return NULL;
        return event;
    }

    /* Parse the stream end. */

    else
    {
        token = yaml_parser_get_token(parser);
        parser->state = YAML_PARSE_END_STATE;
        event = yaml_stream_end_event_new(token->start_mark, token->end_mark);
        yaml_token_delete(token);
        return event;
    }
}

/*
 * Parse the productions:
 * explicit_document    ::= DIRECTIVE* DOCUMENT-START block_node? DOCUMENT-END*
 *                                                    ***********
 */

static yaml_event_t *
yaml_parser_parse_document_content(yaml_parser_t *parser)
{
    yaml_token_t *token;

    token = yaml_parser_peek_token(parser);
    if (!token) return NULL;

    if (token->type == YAML_VERSION_DIRECTIVE_TOKEN ||
            token->type == YAML_TAG_DIRECTIVE_TOKEN ||
            token->type == YAML_DOCUMENT_START_TOKEN ||
            token->type == YAML_DOCUMENT_END_TOKEN ||
            token->type == YAML_STREAM_END_TOKEN) {
        parser->state = parser->states[--parser->states_length];
        return yaml_parser_process_empty_scalar(parser, token->start_mark);
    }
    else {
        return yaml_parser_parse_node(parser, 1, 0);
    }
}

/*
 * Parse the productions:
 * implicit_document    ::= block_node DOCUMENT-END*
 *                                     *************
 * explicit_document    ::= DIRECTIVE* DOCUMENT-START block_node? DOCUMENT-END*
 *                                                                *************
 */

static yaml_event_t *
yaml_parser_parse_document_end(yaml_parser_t *parser)
{
    yaml_token_t *token;
    yaml_event_t *event;
    yaml_mark_t start_mark, end_mark;
    int implicit = 1;

    token = yaml_parser_peek_token(parser);
    if (!token) return NULL;

    start_mark = end_mark = token->start_mark;

    while (token->type == YAML_DOCUMENT_END_TOKEN) {
        end_mark = token->end_mark;
        yaml_token_delete(yaml_parser_get_token(parser));
        token = yaml_parser_peek_token(parser);
        if (!token) return NULL;
        implicit = 0;
    }

    event = yaml_document_end_event_new(implicit, start_mark, end_mark);
    if (!event) {
        parser->error = YAML_MEMORY_ERROR;
        return NULL;
    }
    return event;
}

/*
 * Parse the productions:
 * block_node_or_indentless_sequence    ::=
 *                          ALIAS
 *                          *****
 *                          | properties (block_content | indentless_block_sequence)?
 *                            **********  *
 *                          | block_content | indentless_block_sequence
 *                            *
 * block_node           ::= ALIAS
 *                          *****
 *                          | properties block_content?
 *                            ********** *
 *                          | block_content
 *                            *
 * flow_node            ::= ALIAS
 *                          *****
 *                          | properties flow_content?
 *                            ********** *
 *                          | flow_content
 *                            *
 * properties           ::= TAG ANCHOR? | ANCHOR TAG?
 *                          *************************
 * block_content        ::= block_collection | flow_collection | SCALAR
 *                                                               ******
 * flow_content         ::= flow_collection | SCALAR
 *                                            ******
 */

static yaml_event_t *
yaml_parser_parse_node(yaml_parser_t *parser,
        int block, int indentless_sequence)
{
    yaml_token_t *token;
    yaml_event_t *event;
    yaml_char_t *anchor = NULL;
    yaml_char_t *tag_handle = NULL;
    yaml_char_t *tag_suffix = NULL;
    yaml_char_t *tag = NULL;
    yaml_mark_t start_mark, end_mark, tag_mark;
    int implicit;

    token = yaml_parser_peek_token(parser);
    if (!token) return NULL;

    if (token->type == YAML_ALIAS_TOKEN)
    {
        token = yaml_parser_get_token(parser);
        event = yaml_alias_event_new(token->data.alias.value,
                token->start_mark, token->end_mark);
        if (!event) {
            yaml_token_delete(token);
            parser->error = YAML_MEMORY_ERROR;
            return NULL;
        }
        yaml_free(token);
        return event;
    }

    else
    {
        start_mark = end_mark = token->start_mark;

        if (token->type == YAML_ANCHOR_TOKEN)
        {
            token = yaml_parser_get_token(parser);
            anchor = token->data.anchor.value;
            start_mark = token->start_mark;
            end_mark = token->end_mark;
            yaml_free(token);
            token = yaml_parser_peek_token(parser);
            if (!token) goto error;
            if (token->type == YAML_TAG_TOKEN)
            {
                token = yaml_parser_get_token(parser);
                tag_handle = token->data.tag.handle;
                tag_suffix = token->data.tag.suffix;
                tag_mark = token->start_mark;
                end_mark = token->end_mark;
                yaml_free(token);
                token = yaml_parser_peek_token(parser);
                if (!token) goto error;
            }
        }
        else if (token->type == YAML_TAG_TOKEN)
        {
            token = yaml_parser_get_token(parser);
            tag_handle = token->data.tag.handle;
            tag_suffix = token->data.tag.suffix;
            start_mark = tag_mark = token->start_mark;
            end_mark = token->end_mark;
            yaml_free(token);
            token = yaml_parser_peek_token(parser);
            if (!token) goto error;
            if (token->type == YAML_ANCHOR_TOKEN)
            {
                token = yaml_parser_get_token(parser);
                anchor = token->data.anchor.value;
                end_mark = token->end_mark;
                yaml_free(token);
                token = yaml_parser_peek_token(parser);
                if (!token) goto error;
            }
        }

        if (tag_handle) {
            if (!*tag_handle) {
                tag = tag_suffix;
                yaml_free(tag_handle);
                tag_handle = tag_suffix = NULL;
            }
            else {
                yaml_tag_directive_t **tag_directive = parser->tag_directives;
                for (tag_directive = parser->tag_directives;
                        *tag_directive; tag_directive++) {
                    if (strcmp((char *)(*tag_directive)->handle, (char *)tag_handle) == 0) {
                        size_t prefix_len = strlen((char *)(*tag_directive)->prefix);
                        size_t suffix_len = strlen((char *)tag_suffix);
                        tag = yaml_malloc(prefix_len+suffix_len+1);
                        if (!tag) {
                            parser->error = YAML_MEMORY_ERROR;
                            goto error;
                        }
                        memcpy(tag, (*tag_directive)->handle, prefix_len);
                        memcpy(tag+prefix_len, tag_suffix, suffix_len);
                        tag[prefix_len+suffix_len] = '\0';
                        yaml_free(tag_handle);
                        yaml_free(tag_suffix);
                        tag_handle = tag_suffix = NULL;
                        break;
                    }
                }
                if (*tag_directive) {
                    yaml_parser_set_parser_error_context(parser,
                            "while parsing a node", start_mark,
                            "found undefined tag handle", tag_mark);
                    goto error;
                }
            }
        }

        implicit = (!tag || !*tag);
        if (indentless_sequence && token->type == YAML_BLOCK_ENTRY_TOKEN) {
            end_mark = token->end_mark;
            parser->state = YAML_PARSE_INDENTLESS_SEQUENCE_ENTRY_STATE;
            event = yaml_sequence_start_event_new(anchor, tag, implicit,
                    YAML_BLOCK_SEQUENCE_STYLE, start_mark, end_mark);
            if (!event) goto error;
        }
        else {
            if (token->type == YAML_SCALAR_TOKEN) {
                int plain_implicit = 0;
                int quoted_implicit = 0;
                token = yaml_parser_get_token(parser);
                end_mark = token->end_mark;
                if ((token->data.scalar.style == YAML_PLAIN_SCALAR_STYLE && !tag)
                        || strcmp((char *)tag, "!") == 0) {
                    plain_implicit = 1;
                }
                else if (!tag) {
                    quoted_implicit = 1;
                }
                parser->state = parser->states[--parser->states_length];
                event = yaml_scalar_event_new(anchor, tag,
                        token->data.scalar.value, token->data.scalar.length,
                        plain_implicit, quoted_implicit,
                        token->data.scalar.style, start_mark, end_mark);
                if (!event) {
                    parser->error = YAML_MEMORY_ERROR;
                    yaml_token_delete(token);
                    goto error;
                }
                yaml_free(token);
            }
            else if (token->type == YAML_FLOW_SEQUENCE_START_TOKEN) {
                end_mark = token->end_mark;
                parser->state = YAML_PARSE_FLOW_SEQUENCE_FIRST_ENTRY_STATE;
                event = yaml_sequence_start_event_new(anchor, tag, implicit,
                        YAML_FLOW_SEQUENCE_STYLE, start_mark, end_mark);
                if (!event) {
                    parser->error = YAML_MEMORY_ERROR;
                    goto error;
                }
            }
            else if (token->type == YAML_FLOW_MAPPING_START_TOKEN) {
                end_mark = token->end_mark;
                parser->state = YAML_PARSE_FLOW_MAPPING_FIRST_KEY_STATE;
                event = yaml_mapping_start_event_new(anchor, tag, implicit,
                        YAML_FLOW_MAPPING_STYLE, start_mark, end_mark);
                if (!event) {
                    parser->error = YAML_MEMORY_ERROR;
                    goto error;
                }
            }
            else if (block && token->type == YAML_BLOCK_SEQUENCE_START_TOKEN) {
                end_mark = token->end_mark;
                parser->state = YAML_PARSE_BLOCK_SEQUENCE_FIRST_ENTRY_STATE;
                event = yaml_sequence_start_event_new(anchor, tag, implicit,
                        YAML_BLOCK_SEQUENCE_STYLE, start_mark, end_mark);
                if (!event) {
                    parser->error = YAML_MEMORY_ERROR;
                    goto error;
                }
            }
            else if (block && token->type == YAML_BLOCK_MAPPING_START_TOKEN) {
                end_mark = token->end_mark;
                parser->state = YAML_PARSE_BLOCK_MAPPING_FIRST_KEY_STATE;
                event = yaml_mapping_start_event_new(anchor, tag, implicit,
                        YAML_BLOCK_MAPPING_STYLE, start_mark, end_mark);
                if (!event) {
                    parser->error = YAML_MEMORY_ERROR;
                    goto error;
                }
            }
            else if (anchor || tag) {
                yaml_char_t *value = yaml_malloc(1);
                if (!value) {
                    parser->error = YAML_MEMORY_ERROR;
                    goto error;
                }
                value[0] = '\0';
                event = yaml_scalar_event_new(anchor, tag, value, 0,
                        implicit, 0, YAML_PLAIN_SCALAR_STYLE,
                        start_mark, end_mark);
                if (!event) {
                    yaml_free(value);
                    parser->error = YAML_MEMORY_ERROR;
                    goto error;
                }
            }
            else {
                yaml_parser_set_parser_error_context(parser,
                        (block ? "while parsing a block node"
                         : "while parsing a flow node"), start_mark,
                        "did not found expected node content", token->start_mark);
                goto error;
            }
            return event;
        }
    }

error:
    yaml_free(anchor);
    yaml_free(tag_handle);
    yaml_free(tag_suffix);
    yaml_free(tag);

    return NULL;
}

/*
 * Parse the productions:
 * block_sequence ::= BLOCK-SEQUENCE-START (BLOCK-ENTRY block_node?)* BLOCK-END
 *                    ********************  *********** *             *********
 */

static yaml_event_t *
yaml_parser_parse_block_sequence_entry(yaml_parser_t *parser, int first)
{
    yaml_token_t *token;
    yaml_event_t *event;

    if (first) {
        token = yaml_parser_get_token(parser);
        if (!yaml_parser_append_mark(parser, token->start_mark)) {
            yaml_token_delete(token);
            return NULL;
        }
        yaml_token_delete(token);
    }

    token = yaml_parser_get_token(parser);
    if (!token) return NULL;

    if (token->type == YAML_BLOCK_ENTRY_TOKEN)
    {
        yaml_mark_t mark = token->end_mark;
        yaml_token_delete(token);
        token = yaml_parser_peek_token(parser);
        if (!token) return NULL;
        if (token->type != YAML_BLOCK_ENTRY_TOKEN &&
                token->type != YAML_BLOCK_END_TOKEN) {
            if (!yaml_parser_append_state(parser,
                        YAML_PARSE_BLOCK_SEQUENCE_ENTRY_STATE))
                return NULL;
            return yaml_parser_parse_node(parser, 1, 0);
        }
        else {
            parser->state = YAML_PARSE_BLOCK_SEQUENCE_ENTRY_STATE;
            return yaml_parser_process_empty_scalar(parser, mark);
        }
    }

    else if (token->type == YAML_BLOCK_END_TOKEN)
    {
        parser->state = parser->states[--parser->states_length];
        event = yaml_sequence_end_event_new(token->start_mark, token->end_mark);
        yaml_token_delete(token);
        if (!event) {
            parser->error = YAML_MEMORY_ERROR;
            return NULL;
        }
        return event;
    }

    else
    {
        yaml_parser_set_parser_error_context(parser,
                "while parsing a block collection", parser->marks[parser->marks_length-1],
                "did not found expected '-' indicator", token->start_mark);
        yaml_token_delete(token);
        return NULL;
    }
}

/*
 * Parse the productions:
 * indentless_sequence  ::= (BLOCK-ENTRY block_node?)+
 *                           *********** *
 */

static yaml_event_t *
yaml_parser_parse_indentless_sequence_entry(yaml_parser_t *parser)
{
    yaml_token_t *token;
    yaml_event_t *event;

    token = yaml_parser_peek_token(parser);
    if (!token) return NULL;

    if (token->type == YAML_BLOCK_ENTRY_TOKEN)
    {
        yaml_mark_t mark = token->end_mark;
        yaml_token_delete(yaml_parser_get_token(parser));
        token = yaml_parser_peek_token(parser);
        if (token->type != YAML_BLOCK_ENTRY_TOKEN &&
                token->type != YAML_BLOCK_END_TOKEN) {
            if (!yaml_parser_append_state(parser,
                        YAML_PARSE_INDENTLESS_SEQUENCE_ENTRY_STATE))
                return NULL;
            return yaml_parser_parse_node(parser, 1, 0);
        }
        else {
            parser->state = YAML_PARSE_INDENTLESS_SEQUENCE_ENTRY_STATE;
            return yaml_parser_process_empty_scalar(parser, mark);
        }
    }

    else
    {
        parser->state = parser->states[--parser->states_length];
        event = yaml_sequence_end_event_new(token->start_mark, token->start_mark);
        if (!event) {
            parser->error = YAML_MEMORY_ERROR;
            return NULL;
        }
        return event;
    }
}

/*
 * Parse the productions:
 * block_mapping        ::= BLOCK-MAPPING_START
 *                          *******************
 *                          ((KEY block_node_or_indentless_sequence?)?
 *                            *** *
 *                          (VALUE block_node_or_indentless_sequence?)?)*
 *
 *                          BLOCK-END
 *                          *********
 */

static yaml_event_t *
yaml_parser_parse_block_mapping_key(yaml_parser_t *parser, int first)
{
    yaml_token_t *token;
    yaml_event_t *event;

    if (first) {
        token = yaml_parser_get_token(parser);
        if (!yaml_parser_append_mark(parser, token->start_mark)) {
            yaml_token_delete(token);
            return NULL;
        }
        yaml_token_delete(token);
    }

    token = yaml_parser_get_token(parser);
    if (!token) return NULL;

    if (token->type == YAML_KEY_TOKEN)
    {
        yaml_mark_t mark = token->end_mark;
        yaml_token_delete(token);
        token = yaml_parser_peek_token(parser);
        if (!token) return NULL;
        if (token->type != YAML_KEY_TOKEN &&
                token->type != YAML_VALUE_TOKEN &&
                token->type != YAML_BLOCK_END_TOKEN) {
            if (!yaml_parser_append_state(parser,
                        YAML_PARSE_BLOCK_MAPPING_VALUE_STATE))
                return NULL;
            return yaml_parser_parse_node(parser, 1, 1);
        }
        else {
            parser->state = YAML_PARSE_BLOCK_MAPPING_VALUE_STATE;
            return yaml_parser_process_empty_scalar(parser, mark);
        }
    }

    else if (token->type == YAML_BLOCK_END_TOKEN)
    {
        parser->state = parser->states[--parser->states_length];
        event = yaml_sequence_end_event_new(token->start_mark, token->end_mark);
        yaml_token_delete(token);
        if (!event) {
            parser->error = YAML_MEMORY_ERROR;
            return NULL;
        }
        return event;
    }

    else
    {
        yaml_parser_set_parser_error_context(parser,
                "while parsing a block mapping", parser->marks[parser->marks_length-1],
                "did not found expected key", token->start_mark);
        yaml_token_delete(token);
        return NULL;
    }
}

/*
 * Parse the productions:
 * block_mapping        ::= BLOCK-MAPPING_START
 *
 *                          ((KEY block_node_or_indentless_sequence?)?
 *
 *                          (VALUE block_node_or_indentless_sequence?)?)*
 *                           ***** *
 *                          BLOCK-END
 *
 */

static yaml_event_t *
yaml_parser_parse_block_mapping_value(yaml_parser_t *parser)
{
    yaml_token_t *token;
    yaml_event_t *event;

    token = yaml_parser_peek_token(parser);
    if (!token) return NULL;

    if (token->type == YAML_VALUE_TOKEN)
    {
        yaml_mark_t mark = token->end_mark;
        yaml_token_delete(yaml_parser_get_token(parser));
        token = yaml_parser_peek_token(parser);
        if (!token) return NULL;
        if (token->type != YAML_KEY_TOKEN &&
                token->type != YAML_VALUE_TOKEN &&
                token->type != YAML_BLOCK_END_TOKEN) {
            if (!yaml_parser_append_state(parser,
                        YAML_PARSE_BLOCK_MAPPING_KEY_STATE))
                return NULL;
            return yaml_parser_parse_node(parser, 1, 1);
        }
        else {
            parser->state = YAML_PARSE_BLOCK_MAPPING_KEY_STATE;
            return yaml_parser_process_empty_scalar(parser, mark);
        }
    }

    else
    {
        parser->state = YAML_PARSE_BLOCK_MAPPING_KEY_STATE;
        return yaml_parser_process_empty_scalar(parser, token->start_mark);
    }
}

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry(yaml_parser_t *parser, int first);

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry_mapping_key(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry_mapping_value(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_flow_sequence_entry_mapping_end(yaml_parser_t *parser);

static yaml_event_t *
yaml_parser_parse_flow_mapping_key(yaml_parser_t *parser, int first);

static yaml_event_t *
yaml_parser_parse_flow_mapping_value(yaml_parser_t *parser, int empty);

