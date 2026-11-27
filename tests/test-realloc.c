#include "yaml_private.h"

static int
test_string_extend(void)
{
    const size_t initial_size = 64;
    yaml_char_t *start = yaml_malloc(initial_size);
    yaml_char_t *pointer;
    yaml_char_t *end;

    if (!start)
        return 0;
    pointer = start + 17;
    end = start + initial_size;

    if (!yaml_string_extend(&start, &pointer, &end)) {
        yaml_free(start);
        return 0;
    }
    if ((size_t)(pointer - start) != 17
            || (size_t)(end - start) != initial_size * 2) {
        yaml_free(start);
        return 0;
    }

    yaml_free(start);
    return 1;
}

static int
test_stack_extend(void)
{
    const size_t initial_size = 64;
    char *start = yaml_malloc(initial_size);
    void *storage = start;
    void *top;
    void *end;

    if (!start)
        return 0;
    top = start + 17;
    end = start + initial_size;

    if (!yaml_stack_extend(&storage, &top, &end)) {
        yaml_free(storage);
        return 0;
    }
    start = storage;
    if ((size_t)((char *)top - start) != 17
            || (size_t)((char *)end - start) != initial_size * 2) {
        yaml_free(start);
        return 0;
    }

    yaml_free(start);
    return 1;
}

static int
test_queue_extend(void)
{
    const size_t initial_size = 64;
    char *start = yaml_malloc(initial_size);
    void *storage = start;
    void *head = start;
    void *tail;
    void *end;

    if (!start)
        return 0;
    tail = start + initial_size;
    end = tail;

    if (!yaml_queue_extend(&storage, &head, &tail, &end)) {
        yaml_free(storage);
        return 0;
    }
    start = storage;
    if ((size_t)((char *)head - start) != 0
            || (size_t)((char *)tail - start) != initial_size
            || (size_t)((char *)end - start) != initial_size * 2) {
        yaml_free(start);
        return 0;
    }

    yaml_free(start);
    return 1;
}

int
main(void)
{
    if (!test_string_extend())
        return 1;
    if (!test_stack_extend())
        return 1;
    if (!test_queue_extend())
        return 1;
    return 0;
}
