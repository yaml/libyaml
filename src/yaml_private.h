
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml.h>

#include <assert.h>

/*
 * Memory management.
 */

YAML_DECLARE(void *)
yaml_malloc(size_t size);

YAML_DECLARE(void *)
yaml_realloc(void *ptr, size_t size);

YAML_DECLARE(void)
yaml_free(void *ptr);

YAML_DECLARE(char *)
yaml_strdup(const char *);

/*
 * Reader: Ensure that the buffer contains at least `length` characters.
 */

YAML_DECLARE(int)
yaml_parser_update_buffer(yaml_parser_t *parser, size_t length);

/*
 * Scanner: Ensure that the token stack contains at least one token ready.
 */

YAML_DECLARE(int)
yaml_parser_fetch_more_tokens(yaml_parser_t *parser);

/*
 * The size of the raw buffer.
 */

#define RAW_BUFFER_SIZE 16384

/*
 * The size of the buffer.
 *
 * It should be possible to decode the whole raw buffer.
 */

#define BUFFER_SIZE     (RAW_BUFFER_SIZE*3)

/*
 * The size of other stacks and queues.
 */

#define INITIAL_STACK_SIZE  16
#define INITIAL_QUEUE_SIZE  16
#define INITIAL_STRING_SIZE 16

/*
 * Buffer management.
 */

#define BUFFER_INIT(context,buffer,size)                                        \
    (((buffer).start = yaml_malloc(size)) ?                                     \
        ((buffer).last = (buffer).pointer = (buffer).start,                     \
         (buffer).end = (buffer).start+(size),                                  \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

#define BUFFER_DEL(context,buffer)                                              \
    (yaml_free((buffer).start),                                                 \
     (buffer).start = (buffer).pointer = (buffer).end = 0)

/*
 * String management.
 */

typedef struct {
    yaml_char_t *start;
    yaml_char_t *end;
    yaml_char_t *pointer;
} yaml_string_t;

YAML_DECLARE(int)
yaml_string_extend(yaml_char_t **start,
        yaml_char_t **pointer, yaml_char_t **end);

YAML_DECLARE(int)
yaml_string_join(
        yaml_char_t **a_start, yaml_char_t **a_pointer, yaml_char_t **a_end,
        yaml_char_t **b_start, yaml_char_t **b_pointer, yaml_char_t **b_end);

#define NULL_STRING { NULL, NULL, NULL }

#define STRING_INIT(context,string,size)                                        \
    (((string).start = yaml_malloc(size)) ?                                     \
        ((string).pointer = (string).start,                                     \
         (string).end = (string).start+(size),                                  \
         memset((string).start, 0, (size)),                                     \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

#define STRING_DEL(context,string)                                              \
    (yaml_free((string).start),                                                 \
     (string).start = (string).pointer = (string).end = 0)

#define STRING_EXTEND(context,string)                                           \
    (((string).pointer+5 < (string).end)                                        \
        || yaml_string_extend(&(string).start,                                  \
            &(string).pointer, &(string).end))

#define CLEAR(context,string)                                                   \
    ((string).pointer = (string).start,                                         \
     memset((string).start, 0, (string).end-(string).start))

#define JOIN(context,string_a,string_b)                                         \
    ((yaml_string_join(&(string_a).start, &(string_a).pointer,                  \
                       &(string_a).end, &(string_b).start,                      \
                       &(string_b).pointer, &(string_b).end)) ?                 \
        ((string_b).pointer = (string_b).start,                                 \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

/*
 * Stack and queue management.
 */

YAML_DECLARE(int)
yaml_stack_extend(void **start, void **top, void **end);

YAML_DECLARE(int)
yaml_queue_extend(void **start, void **head, void **tail, void **end);

#define STACK_INIT(context,stack,size)                                          \
    (((stack).start = yaml_malloc((size)*sizeof(*(stack).start))) ?             \
        ((stack).top = (stack).start,                                           \
         (stack).end = (stack).start+(size),                                    \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

#define STACK_DEL(context,stack)                                                \
    (yaml_free((stack).start),                                                  \
     (stack).start = (stack).top = (stack).end = 0)

#define STACK_EMPTY(context,stack)                                              \
    ((stack).start == (stack).top)

#define PUSH(context,stack,value)                                               \
    (((stack).top != (stack).end                                                \
      || yaml_stack_extend((void **)&(stack).start,                             \
              (void **)&(stack).top, (void **)&(stack).end)) ?                  \
        (*((stack).top++) = value,                                              \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

#define POP(context,stack)                                                      \
    (*(--(stack).top))

#define QUEUE_INIT(context,queue,size)                                          \
    (((queue).start = yaml_malloc((size)*sizeof(*(queue).start))) ?             \
        ((queue).head = (queue).tail = (queue).start,                           \
         (queue).end = (queue).start+(size),                                    \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

#define QUEUE_DEL(context,queue)                                                \
    (yaml_free((queue).start),                                                  \
     (queue).start = (queue).head = (queue).tail = (queue).end = 0)

#define QUEUE_EMPTY(context,queue)                                              \
    ((queue).head == (queue).tail)

#define ENQUEUE(context,queue,value)                                            \
    (((queue).tail != (queue).end                                               \
      || yaml_queue_extend((void **)&(queue).start, (void **)&(queue).head,     \
            (void **)&(queue).tail, (void **)&(queue).end)) ?                   \
        (*((queue).tail++) = value,                                             \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

#define DEQUEUE(context,queue)                                                  \
    (*((queue).head++))

#define QUEUE_INSERT(context,queue,index,value)                                 \
    (((queue).tail != (queue).end                                               \
      || yaml_queue_extend((void **)&(queue).start, (void **)&(queue).head,     \
            (void **)&(queue).tail, (void **)&(queue).end)) ?                   \
        (memmove((queue).head+(index)+1,(queue).head+(index),                   \
            ((queue).tail-(queue).head-(index))*sizeof(*(queue).start)),        \
         *((queue).head+(index)) = value,                                       \
         (queue).tail++,                                                        \
         1) :                                                                   \
        ((context)->error = YAML_MEMORY_ERROR,                                  \
         0))

