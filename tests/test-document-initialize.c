#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

/*
 * yaml_document_initialize() can fail partway through its work (for
 * example when a supplied tag directive is not valid UTF-8) without ever
 * writing to the caller's yaml_document_t.  Unlike yaml_parser_initialize()
 * and yaml_emitter_initialize(), which both start by zeroing the object
 * they are given, yaml_document_initialize() used to leave the document
 * untouched on such a failure.  A document that was never zeroed (e.g. a
 * plain stack-local yaml_document_t, as used by callers that mirror the
 * pattern of every other *_initialize() function in this library) is then
 * unsafe to pass to yaml_document_delete(), which will dereference
 * whatever garbage happens to be in document->nodes and friends.
 *
 * This test exercises exactly that failure path and confirms that the
 * document is left in a safely-deletable (zeroed) state.
 */

int
main(void)
{
    yaml_document_t document;
    yaml_tag_directive_t bad_tag_directives[1];
    yaml_document_t zeroed;
    int ok;

    /* Poison the document with a non-zero pattern first, the same way an
     * uninitialized stack variable would look in practice, so that any
     * leftover wild pointer is easy to spot if it is ever dereferenced. */
    memset(&document, 0xAA, sizeof(document));

    /* A tag directive whose prefix is not valid UTF-8 (a lone trailing
     * continuation byte) makes yaml_document_initialize() fail inside its
     * tag directive validation loop, well before it ever populates
     * *document on the success path. */
    bad_tag_directives[0].handle = (yaml_char_t *)"!bad!";
    bad_tag_directives[0].prefix = (yaml_char_t *)"\x80";

    ok = yaml_document_initialize(&document, NULL,
            bad_tag_directives, bad_tag_directives + 1, 0, 0);
    assert(!ok);

    /* The document must be left in a well-defined, all-zero state so that
     * it is safe to hand to yaml_document_delete(), just like every other
     * *_initialize() function in this library guarantees on failure. */
    memset(&zeroed, 0, sizeof(zeroed));
    assert(memcmp(&document, &zeroed, sizeof(document)) == 0);

    /* This must not crash. Before the fix, the document above was left
     * with whatever garbage the 0xAA poisoning put in it, and deleting it
     * would try to free wild pointers. */
    yaml_document_delete(&document);

    printf("test-document-initialize: ok\n");

    return 0;
}
