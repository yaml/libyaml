
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

