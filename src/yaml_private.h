
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
 * The size of the input raw buffer.
 */

#define INPUT_RAW_BUFFER_SIZE   16384

/*
 * The size of the input buffer.
 *
 * It should be possible to decode the whole raw buffer.
 */

#define INPUT_BUFFER_SIZE       (INPUT_RAW_BUFFER_SIZE*3)

/*
 * The size of the output buffer.
 */

#define OUTPUT_BUFFER_SIZE      16384

/*
 * The size of the output raw buffer.
 *
 * It should be possible to encode the whole output buffer.
 */

#define OUTPUT_RAW_BUFFER_SIZE  (OUTPUT_BUFFER_SIZE*2+2)

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

/*
 * Token initializers.
 */

#define TOKEN_INIT(token,token_type,token_start_mark,token_end_mark)            \
    (memset(&(token), 0, sizeof(yaml_token_t)),                                 \
     (token).type = (token_type),                                               \
     (token).start_mark = (token_start_mark),                                   \
     (token).end_mark = (token_end_mark))

#define STREAM_START_TOKEN_INIT(token,token_encoding,start_mark,end_mark)       \
    (TOKEN_INIT((token),YAML_STREAM_START_TOKEN,(start_mark),(end_mark)),       \
     (token).data.stream_start.encoding = (token_encoding))

#define STREAM_END_TOKEN_INIT(token,start_mark,end_mark)                        \
    (TOKEN_INIT((token),YAML_STREAM_END_TOKEN,(start_mark),(end_mark)))

#define ALIAS_TOKEN_INIT(token,token_value,start_mark,end_mark)                 \
    (TOKEN_INIT((token),YAML_ALIAS_TOKEN,(start_mark),(end_mark)),              \
     (token).data.alias.value = (token_value))

#define ANCHOR_TOKEN_INIT(token,token_value,start_mark,end_mark)                \
    (TOKEN_INIT((token),YAML_ANCHOR_TOKEN,(start_mark),(end_mark)),             \
     (token).data.anchor.value = (token_value))

#define TAG_TOKEN_INIT(token,token_handle,token_suffix,start_mark,end_mark)     \
    (TOKEN_INIT((token),YAML_TAG_TOKEN,(start_mark),(end_mark)),                \
     (token).data.tag.handle = (token_handle),                                  \
     (token).data.tag.suffix = (token_suffix))

#define SCALAR_TOKEN_INIT(token,token_value,token_length,token_style,start_mark,end_mark)   \
    (TOKEN_INIT((token),YAML_SCALAR_TOKEN,(start_mark),(end_mark)),             \
     (token).data.scalar.value = (token_value),                                 \
     (token).data.scalar.length = (token_length),                               \
     (token).data.scalar.style = (token_style))

#define VERSION_DIRECTIVE_TOKEN_INIT(token,token_major,token_minor,start_mark,end_mark)     \
    (TOKEN_INIT((token),YAML_VERSION_DIRECTIVE_TOKEN,(start_mark),(end_mark)),  \
     (token).data.version_directive.major = (token_major),                      \
     (token).data.version_directive.minor = (token_minor))

#define TAG_DIRECTIVE_TOKEN_INIT(token,token_handle,token_prefix,start_mark,end_mark)       \
    (TOKEN_INIT((token),YAML_TAG_DIRECTIVE_TOKEN,(start_mark),(end_mark)),      \
     (token).data.tag_directive.handle = (token_handle),                        \
     (token).data.tag_directive.prefix = (token_prefix))

/*
 * Event initializers.
 */

#define EVENT_INIT(event,event_type,event_start_mark,event_end_mark)            \
    (memset(&(event), 0, sizeof(yaml_event_t)),                                 \
     (event).type = (event_type),                                               \
     (event).start_mark = (event_start_mark),                                   \
     (event).end_mark = (event_end_mark))

#define STREAM_START_EVENT_INIT(event,event_encoding,start_mark,end_mark)       \
    (EVENT_INIT((event),YAML_STREAM_START_EVENT,(start_mark),(end_mark)),       \
     (event).data.stream_start.encoding = (event_encoding))

#define STREAM_END_EVENT_INIT(event,start_mark,end_mark)                        \
    (EVENT_INIT((event),YAML_STREAM_END_EVENT,(start_mark),(end_mark)))

#define DOCUMENT_START_EVENT_INIT(event,event_version_directive,                \
        event_tag_directives_start,event_tag_directives_end,event_implicit,start_mark,end_mark) \
    (EVENT_INIT((event),YAML_DOCUMENT_START_EVENT,(start_mark),(end_mark)),     \
     (event).data.document_start.version_directive = (event_version_directive), \
     (event).data.document_start.tag_directives.start = (event_tag_directives_start),   \
     (event).data.document_start.tag_directives.end = (event_tag_directives_end),   \
     (event).data.document_start.implicit = (event_implicit))

#define DOCUMENT_END_EVENT_INIT(event,event_implicit,start_mark,end_mark)       \
    (EVENT_INIT((event),YAML_DOCUMENT_END_EVENT,(start_mark),(end_mark)),       \
     (event).data.document_end.implicit = (event_implicit))

#define ALIAS_EVENT_INIT(event,event_anchor,start_mark,end_mark)                \
    (EVENT_INIT((event),YAML_ALIAS_EVENT,(start_mark),(end_mark)),              \
     (event).data.alias.anchor = (event_anchor))

#define SCALAR_EVENT_INIT(event,event_anchor,event_tag,event_value,event_length,    \
        event_plain_implicit, event_quoted_implicit,event_style,start_mark,end_mark)    \
    (EVENT_INIT((event),YAML_SCALAR_EVENT,(start_mark),(end_mark)),             \
     (event).data.scalar.anchor = (event_anchor),                               \
     (event).data.scalar.tag = (event_tag),                                     \
     (event).data.scalar.value = (event_value),                                 \
     (event).data.scalar.length = (event_length),                               \
     (event).data.scalar.plain_implicit = (event_plain_implicit),               \
     (event).data.scalar.quoted_implicit = (event_quoted_implicit),             \
     (event).data.scalar.style = (event_style))

#define SEQUENCE_START_EVENT_INIT(event,event_anchor,event_tag,                 \
        event_implicit,event_style,start_mark,end_mark)                         \
    (EVENT_INIT((event),YAML_SEQUENCE_START_EVENT,(start_mark),(end_mark)),     \
     (event).data.sequence_start.anchor = (event_anchor),                       \
     (event).data.sequence_start.tag = (event_tag),                             \
     (event).data.sequence_start.implicit = (event_implicit),                   \
     (event).data.sequence_start.style = (event_style))

#define SEQUENCE_END_EVENT_INIT(event,start_mark,end_mark)                      \
    (EVENT_INIT((event),YAML_SEQUENCE_END_EVENT,(start_mark),(end_mark)))

#define MAPPING_START_EVENT_INIT(event,event_anchor,event_tag,                  \
        event_implicit,event_style,start_mark,end_mark)                         \
    (EVENT_INIT((event),YAML_MAPPING_START_EVENT,(start_mark),(end_mark)),      \
     (event).data.mapping_start.anchor = (event_anchor),                        \
     (event).data.mapping_start.tag = (event_tag),                              \
     (event).data.mapping_start.implicit = (event_implicit),                    \
     (event).data.mapping_start.style = (event_style))

#define MAPPING_END_EVENT_INIT(event,start_mark,end_mark)                       \
    (EVENT_INIT((event),YAML_MAPPING_END_EVENT,(start_mark),(end_mark)))

