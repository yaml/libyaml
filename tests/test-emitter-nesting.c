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
 * stack is exhausted (CWE-674).
 *
 * The document is `depth` nested sequences deep:  [ [ [ ... [ "x" ] ... ] ] ]
 */

static int
dump_nested_sequence(int depth)
{
    yaml_emitter_t emitter;
    yaml_document_t document;
    FILE *sink;
    int root, prev, cur, leaf, i, rc;

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

    assert(yaml_emitter_initialize(&emitter));
    sink = fopen("/dev/null", "wb");
    assert(sink);
    yaml_emitter_set_output_file(&emitter, sink);

    /* 1 on success, 0 on (graceful) failure -- never a crash. */
    rc = yaml_emitter_dump(&emitter, &document);

    yaml_emitter_delete(&emitter);
    fclose(sink);
    return rc;
}

int
main(void)
{
    /* Test 1: a shallow document dumps successfully. */
    printf("Test 1: dump 100-deep sequence (within limit) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(100) == 1);
    printf("OK (emitted)\n");

    /* Test 2: a document nested far beyond the limit must fail GRACEFULLY
     * (return 0) rather than exhausting the C stack and crashing. The default
     * limit is 1000; 100000 is well past the ~tens-of-thousands crash point of
     * the previously unbounded recursion. */
    printf("Test 2: dump 100000-deep sequence (exceeds limit) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(100000) == 0);
    printf("OK (rejected, no crash)\n");

    /* Test 3: the limit honors yaml_set_max_nest_level(). */
    yaml_set_max_nest_level(100);
    printf("Test 3: limit=100, dump 50-deep (within) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(50) == 1);
    printf("OK (emitted)\n");
    printf("Test 3: limit=100, dump 500-deep (exceeds) ... ");
    fflush(stdout);
    assert(dump_nested_sequence(500) == 0);
    printf("OK (rejected, no crash)\n");

    return 0;
}
