#include <yaml.h>

#include <limits.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

int
main(void)
{
    yaml_document_t document;
    yaml_node_t *sequence_node, *mapping_node;
    int scalar, sequence, mapping, item;

    assert(yaml_document_initialize(&document, NULL, NULL, NULL, 0, 0));
    scalar = yaml_document_add_scalar(&document, NULL,
            (yaml_char_t *)"key", -1, YAML_PLAIN_SCALAR_STYLE);
    sequence = yaml_document_add_sequence(&document, NULL,
            YAML_BLOCK_SEQUENCE_STYLE);
    mapping = yaml_document_add_mapping(&document, NULL,
            YAML_BLOCK_MAPPING_STYLE);
    item = yaml_document_add_scalar(&document, NULL,
            (yaml_char_t *)"value", -1, YAML_PLAIN_SCALAR_STYLE);
    assert(scalar && sequence && mapping && item);

    sequence_node = yaml_document_get_node(&document, sequence);
    mapping_node = yaml_document_get_node(&document, mapping);

    assert(yaml_document_get_node(&document, scalar));
    assert(!yaml_document_get_node(&document, 0));
    assert(!yaml_document_get_node(&document, INT_MAX));

    assert(!yaml_document_append_sequence_item(&document, 0, item));
    assert(!yaml_document_append_sequence_item(&document, scalar, item));
    assert(!yaml_document_append_sequence_item(&document, sequence, 0));
    assert(!yaml_document_append_sequence_item(&document,
                sequence, INT_MAX));
    assert(sequence_node->data.sequence.items.top
            == sequence_node->data.sequence.items.start);
    assert(yaml_document_append_sequence_item(&document, sequence, item));
    assert(sequence_node->data.sequence.items.top
            - sequence_node->data.sequence.items.start == 1);

    assert(!yaml_document_append_mapping_pair(&document,
                0, scalar, item));
    assert(!yaml_document_append_mapping_pair(&document,
                sequence, scalar, item));
    assert(!yaml_document_append_mapping_pair(&document,
                mapping, 0, item));
    assert(!yaml_document_append_mapping_pair(&document,
                mapping, scalar, INT_MAX));
    assert(mapping_node->data.mapping.pairs.top
            == mapping_node->data.mapping.pairs.start);
    assert(yaml_document_append_mapping_pair(&document,
                mapping, scalar, item));
    assert(mapping_node->data.mapping.pairs.top
            - mapping_node->data.mapping.pairs.start == 1);

    yaml_document_delete(&document);
    return 0;
}
