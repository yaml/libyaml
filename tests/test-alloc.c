/* Counting/fault-injecting allocator implementation. Compiled without the
 * malloc/realloc/free renaming, so it can forward to the real allocator. */
#include "test-alloc.h"

#include <stdlib.h>
#include <string.h>

long test_alloc_calls = 0;
long test_alloc_outstanding = 0;
long test_alloc_fail_at = -1;

void *test_malloc(size_t size) {
    void *ptr;

    test_alloc_calls++;
    if (test_alloc_fail_at >= 0 && test_alloc_calls == test_alloc_fail_at) {
        return NULL;
    }
    ptr = malloc(size);
    if (ptr) {
        test_alloc_outstanding++;
    }
    return ptr;
}

void *test_realloc(void *ptr, size_t size) {
    void *new_ptr;

    test_alloc_calls++;
    if (test_alloc_fail_at >= 0 && test_alloc_calls == test_alloc_fail_at) {
        return NULL;
    }
    new_ptr = realloc(ptr, size);
    if (new_ptr && !ptr) {
        test_alloc_outstanding++;
    }
    return new_ptr;
}

void test_free(void *ptr) {
    if (ptr) {
        test_alloc_outstanding--;
    }
    free(ptr);
}

/* strdup() allocates behind the library's back; route it through the
 * counting allocator as well. */
char *test_strdup(const char *s) {
    size_t len = strlen(s) + 1;
    char *copy = (char *)test_malloc(len);

    if (copy) {
        memcpy(copy, s, len);
    }
    return copy;
}

void test_alloc_reset(void) {
    test_alloc_calls = 0;
    test_alloc_outstanding = 0;
    test_alloc_fail_at = -1;
}
