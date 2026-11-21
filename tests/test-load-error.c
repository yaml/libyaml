/* Regression test for allocation-failure handling in the loader.
 *
 * Runs yaml_parser_load over documents that place mapping/sequence nodes
 * on the document node-stack growth boundaries (node 17, 33, ...). When
 * PUSH grows the node stack and the growth allocation fails, the freshly
 * allocated node pairs/items array is not attached to the document yet,
 * and the loader's error path must free it. Before that fix, these runs
 * leaked one block each, which this test reports.
 *
 * The test fails one allocation per run, exhaustively, and verifies that
 * every failure path leaves zero outstanding blocks.
 */
#include "test-alloc.h"

#include <stdio.h>
#include <string.h>
#include <yaml.h>

/* 24 empty mappings as sequence items: the root sequence is node 1 and
 * item N is node N+1, so items 16 and 32 are mapping nodes composed on
 * node-stack growth boundaries with a live pairs array. */
static const char test_mappings[] =
    "- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n"
    "- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n"
    "- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n- {}\n";

/* Same layout for sequences: items 16 and 32 are sequence nodes with a
 * live items array. */
static const char test_sequences[] =
    "- []\n- []\n- []\n- []\n- []\n- []\n- []\n- []\n"
    "- []\n- []\n- []\n- []\n- []\n- []\n- []\n- []\n"
    "- []\n- []\n- []\n- []\n- []\n- []\n- []\n- []\n";

/* Load one document (or fail partway), then release everything the API
 * contract leaves to the caller. Returns 1 on success, 0 on failure. */
static int run_load(const char *doc, size_t len) {
    yaml_parser_t parser;
    yaml_document_t document;
    int ok = 0;

    if (!yaml_parser_initialize(&parser)) {
        return 0;
    }
    yaml_parser_set_input_string(&parser, (const unsigned char *)doc, len);
    if (yaml_parser_load(&parser, &document)) {
        yaml_document_delete(&document);
        ok = 1;
    }
    yaml_parser_delete(&parser);
    return ok;
}

static int run_test(const char *name, const char *doc) {
    size_t len = strlen(doc);
    long fail_at;
    long total_calls;

    /* Clean run to determine the total number of allocation calls. */
    test_alloc_reset();
    if (!run_load(doc, len)) {
        fprintf(stderr, "test-load-error: %s: clean run failed\n", name);
        return 1;
    }
    if (test_alloc_outstanding != 0) {
        fprintf(stderr, "test-load-error: %s: %ld block(s) leaked on success path\n",
                name, test_alloc_outstanding);
        return 1;
    }
    total_calls = test_alloc_calls;

    for (fail_at = 1; fail_at <= total_calls; fail_at++) {
        test_alloc_reset();
        test_alloc_fail_at = fail_at;
        run_load(doc, len); /* failure here is expected and fine */
        test_alloc_fail_at = -1;

        if (test_alloc_outstanding != 0) {
            fprintf(stderr,
                    "test-load-error: %s: %ld block(s) leaked when allocation %ld failed\n",
                    name, test_alloc_outstanding, fail_at);
            return 1;
        }
    }

    printf("test-load-error: %s: OK (%ld failure points)\n", name, total_calls);
    return 0;
}

int main(void) {
    int failed = 0;

    failed |= run_test("mappings", test_mappings);
    failed |= run_test("sequences", test_sequences);
    return failed;
}
