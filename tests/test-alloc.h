/* Counting/fault-injecting allocator used by test-load-error.
 *
 * The library sources are compiled with malloc/realloc/free renamed to
 * test_malloc/test_realloc/test_free, while this implementation is compiled
 * normally and forwards to the real allocator. That keeps the interception
 * portable (no LD_PRELOAD, no linker tricks).
 */
#ifndef TEST_ALLOC_H
#define TEST_ALLOC_H

#include <stddef.h>

extern long test_alloc_calls;    /* total malloc+realloc calls          */
extern long test_alloc_outstanding; /* live blocks (alloc - free)       */
extern long test_alloc_fail_at;  /* fail the Nth call; -1 disables     */

void *test_malloc(size_t size);
void *test_realloc(void *ptr, size_t size);
void test_free(void *ptr);
char *test_strdup(const char *s);

void test_alloc_reset(void);

#endif /* TEST_ALLOC_H */
