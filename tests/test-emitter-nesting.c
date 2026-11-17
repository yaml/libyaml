#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

/*
 * Test that the emitter enforces the maximum nesting depth when dumping a
 * document built through the document API, instead of recursing until the C
 * stack is exhausted (CWE-674), and that it uses the same depth semantics as
 * the parser so anything the parser loads can be dumped at the same limit.
 */

static int
discard_output(void *data, unsigned char *buffer, size_t size)
{
    (void)data;
    (void)buffer;
    (void)size;
    return 1;
}

static int
dump_document(yaml_document_t *document)
{
    yaml_emitter_t emitter;
    int rc;

    assert(yaml_emitter_initialize(&emitter));
    yaml_emitter_set_output(&emitter, discard_output, NULL);

    rc = yaml_emitter_dump(&emitter, document);

    yaml_emitter_delete(&emitter);
    return rc;
}

static int
dump_nested_sequence(int depth)
{
    yaml_document_t document;
    int root, prev, cur, leaf, i;

    assert(yaml_document_initialize(&document, NULL, NULL, NULL, 1, 1));

    root = yaml_document_add_sequence(&document, NULL,
            YAML_BLOCK_SEQUENCE_STYLE);
    assert(root);
    prev = root;
    for (i = 1; i < depth; i++) {
        cur = yaml_document_add_sequence(&document, NULL,
                YAML_BLOCK_SEQUENCE_STYLE);
        assert(cur);
        assert(yaml_document_append_sequence_item(&document, prev, cur));
        prev = cur;
    }
    leaf = yaml_document_add_scalar(&document, NULL,
            (yaml_char_t *)"x", 1, YAML_PLAIN_SCALAR_STYLE);
    assert(leaf);
    assert(yaml_document_append_sequence_item(&document, prev, leaf));

    /* yaml_emitter_dump() deletes the document. */
    return dump_document(&document);
}

/*
 * Load `input` with the parser and dump the result.  Returns -1 if the
 * parser rejects the input, otherwise the result of yaml_emitter_dump().
 */

static int
load_and_dump(const char *input)
{
    yaml_parser_t parser;
    yaml_document_t document;
    int loaded;

    assert(yaml_parser_initialize(&parser));
    yaml_parser_set_input_string(&parser,
            (const unsigned char *)input, strlen(input));
    loaded = yaml_parser_load(&parser, &document);
    yaml_parser_delete(&parser);

    if (!loaded)
        return -1;
    assert(yaml_document_get_root_node(&document));

    return dump_document(&document);
}

int
main(void)
{
    int default_limit = yaml_get_max_nest_level();

    printf("Test 1: dump 100-deep sequence (within limit) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(100) == 1);
    printf("OK (emitted)\n");

    /* The default limit is 1000; 100000 is well past the ~tens-of-thousands
     * crash point of the previously unbounded recursion. */
    printf("Test 2: dump 100000-deep sequence (exceeds limit) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(100000) == 0);
    printf("OK (rejected, no crash)\n");

    yaml_set_max_nest_level(100);
    printf("Test 3: limit=100, dump 50-deep (within) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(50) == 1);
    printf("OK (emitted)\n");
    printf("Test 3: limit=100, dump 500-deep (exceeds) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(500) == 0);
    printf("OK (rejected, no crash)\n");

    /* Exactly at the limit: the scalar leaf does not count as a level. */
    yaml_set_max_nest_level(3);
    printf("Test 4: limit=3, dump 3-deep sequence with scalar leaf ... ");
    fflush(stdout);
    assert(dump_nested_sequence(3) == 1);
    printf("OK (emitted)\n");
    printf("Test 4: limit=3, dump 4-deep sequence ... ");
    fflush(stdout);
    assert(dump_nested_sequence(4) == 0);
    printf("OK (rejected)\n");
    printf("Test 4: limit=3, load and dump [[[x]]] ... ");
    fflush(stdout);
    assert(load_and_dump("[[[x]]]") == 1);
    printf("OK (emitted)\n");

    /* An alias back to an already visited node is emitted without recursing
     * and must not count as another level. */
    yaml_set_max_nest_level(1);
    printf("Test 5: limit=1, load and dump cyclic &a [*a] ... ");
    fflush(stdout);
    assert(load_and_dump("&a [*a]") == 1);
    printf("OK (emitted)\n");
    printf("Test 5: limit=1, load and dump shared alias [&a x, *a] ... ");
    fflush(stdout);
    assert(load_and_dump("[&a x, *a]") == 1);
    printf("OK (emitted)\n");

    yaml_set_max_nest_level(2);
    printf("Test 6: limit=2, load and dump shared collection "
            "{a: &s [x], b: *s} ... ");
    fflush(stdout);
    assert(load_and_dump("{a: &s [x], b: *s}") == 1);
    printf("OK (emitted)\n");

    yaml_set_max_nest_level(default_limit);

    return 0;
}
