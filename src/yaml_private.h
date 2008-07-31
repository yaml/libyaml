/*****************************************************************************
 * Private Definitions for LibYAML
 *
 * Copyright (c) 2006 Kirill Simonov
 *
 * LibYAML is free software; you can use, modify and/or redistribute it under
 * the terms of the MIT license; see the file LICENCE for more details.
 *****************************************************************************/

#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml.h>

#include <assert.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <locale.h>

/*****************************************************************************
 * Memory Management
 *****************************************************************************/

YAML_DECLARE(void *)
yaml_malloc(size_t size);

YAML_DECLARE(void *)
yaml_realloc(void *ptr, size_t size);

YAML_DECLARE(void)
yaml_free(void *ptr);

YAML_DECLARE(yaml_char_t *)
yaml_strdup(const yaml_char_t *);

/*****************************************************************************
 * Error Management
 *****************************************************************************/

/*
 * Generic error initializers; not to be used directly.
 */

#define ERROR_INIT(error, _type)                                                \
    (memset(&(error), 0, sizeof(yaml_error_t)),                                 \
     (error).type = (_type),                                                    \
     0)

#define READING_ERROR_INIT(error, _type, _problem, _offset, _value)             \
    (ERROR_INIT(error, _type),                                                  \
     (error).data.reading.problem = (_problem),                                 \
     (error).data.reading.offset = (_offset),                                   \
     (error).data.reading.value = (_value),                                     \
     0)

#define LOADING_ERROR_INIT(error, _type, _problem, _problem_mark)               \
    (ERROR_INIT(error, _type),                                                  \
     (error).data.loading.context = NULL,                                       \
     (error).data.loading.context_mark.index = 0,                               \
     (error).data.loading.context_mark.line = 0,                                \
     (error).data.loading.context_mark.column = 0,                              \
     (error).data.loading.problem = (_problem),                                 \
     (error).data.loading.problem_mark = (_problem_mark),                       \
     0)

#define LOADING_ERROR_WITH_CONTEXT_INIT(error, _type, _context, _context_mark,  \
        _problem, _problem_mark)                                                \
    (ERROR_INIT(error, _type),                                                  \
     (error).data.loading.context = (_context),                                 \
     (error).data.loading.context_mark = (_context_mark),                       \
     (error).data.loading.problem = (_problem),                                 \
     (error).data.loading.problem_mark = (_problem_mark),                       \
     0)

#define WRITING_ERROR_INIT(error, _type, _problem, _offset)                     \
    (ERROR_INIT(error, _type),                                                  \
     (error).data.writing.problem = (_problem),                                 \
     (error).data.writing.offset = (_offset),                                   \
     0)

#define DUMPING_ERROR_INIT(error, _type, _problem)                              \
    (ERROR_INIT(error, _type),                                                  \
     (error).data.dumping.problem = (_problem),                                 \
     0)

#define RESOLVING_ERROR_INIT(error, _type, _problem)                            \
    (ERROR_INIT(error, _type),                                                  \
     (error).data.resolving.problem = (_problem),                               \
     0)

/*
 * Specific error initializers.
 */

#define MEMORY_ERROR_INIT(self)                                                 \
    ERROR_INIT((self)->error, YAML_MEMORY_ERROR)

#define READER_ERROR_INIT(self, _problem, _offset)                              \
    READING_ERROR_INIT((self)->error, YAML_READER_ERROR, _problem, _offset, -1)

#define DECODER_ERROR_INIT(self, _problem, _offset, _value)                     \
    READING_ERROR_INIT((self)->error, YAML_DECODER_ERROR, _problem, _offset, _value)

#define SCANNER_ERROR_INIT(self, _problem, _problem_mark)                       \
    LOADING_ERROR_INIT((self)->error, YAML_SCANNER_ERROR, _problem, _problem_mark)

#define SCANNER_ERROR_WITH_CONTEXT_INIT(self, _context, _context_mark,          \
        _problem, _problem_mark)                                                \
    LOADING_ERROR_WITH_CONTEXT_INIT((self)->error, YAML_SCANNER_ERROR,          \
            _context, _context_mark, _problem, _problem_mark)

#define PARSER_ERROR_INIT(self, _problem, _problem_mark)                        \
    LOADING_ERROR_INIT((self)->error, YAML_PARSER_ERROR, _problem, _problem_mark)

#define PARSER_ERROR_WITH_CONTEXT_INIT(self, _context, _context_mark,           \
        _problem, _problem_mark)                                                \
    LOADING_ERROR_WITH_CONTEXT_INIT((self)->error, YAML_PARSER_ERROR,           \
            _context, _context_mark, _problem, _problem_mark)

#define COMPOSER_ERROR_INIT(self, _problem, _problem_mark)                      \
    LOADING_ERROR_INIT((self)->error, YAML_COMPOSER_ERROR, _problem, _problem_mark)

#define COMPOSER_ERROR_WITH_CONTEXT_INIT(self, _context, _context_mark,         \
        _problem, _problem_mark)                                                \
    LOADING_ERROR_WITH_CONTEXT_INIT((self)->error, YAML_COMPOSER_ERROR,         \
            _context, _context_mark, _problem, _problem_mark)

#define WRITER_ERROR_INIT(self, _problem, _offset)                              \
    WRITING_ERROR_INIT((self)->error, YAML_WRITER_ERROR, _problem, _offset)

#define EMITTER_ERROR_INIT(self, _problem)                                      \
    DUMPING_ERROR_INIT((self)->error, YAML_EMITTER_ERROR, _problem)

#define SERIALIZER_ERROR_INIT(self, _problem)                                   \
    DUMPING_ERROR_INIT((self)->error, YAML_SERIALIZER_ERROR, _problem)

#define RESOLVER_ERROR_INIT(self, _problem)                                     \
    RESOLVER_ERROR_INIT((self)->error, YAML_RESOLVER_ERROR, _problem)

/*****************************************************************************
 * Buffer Sizes
 *****************************************************************************/

/*
 * The size of the input raw buffer.
 */

#define RAW_INPUT_BUFFER_CAPACITY   16384

/*
 * The size of the input buffer.
 *
 * The input buffer should be large enough to hold the content of the raw
 * buffer after it is decoded.  The raw input buffer could be encoded in UTF-8
 * or UTF-16 while the input buffer is always encoded in UTF-8.  A UTF-16
 * character may take 2 or 4 bytes, and a UTF-8 character takes up to 4 bytes.
 * We use the *3 multiplier just to be safe.
 */

#define INPUT_BUFFER_CAPACITY   (RAW_INPUT_BUFFER_CAPACITY*3)

/*
 * The size of the output buffer.
 */

#define OUTPUT_BUFFER_CAPACITY  16384

/*
 * The size of the output raw buffer.
 *
 * The raw buffer should be able to hold the content of the output buffer
 * after it is encoded.
 */

#define RAW_OUTPUT_BUFFER_CAPACITY  (OUTPUT_BUFFER_CAPACITY*2+2)

/*
 * The size of other stacks and queues.
 */

#define INITIAL_STACK_CAPACITY  16
#define INITIAL_QUEUE_CAPACITY  16
#define INITIAL_STRING_CAPACITY 16

/*****************************************************************************
 * String Management
 *****************************************************************************/

/*
 * An immutable string used as an input buffer.
 */

typedef struct yaml_istring_s {
    const yaml_char_t *buffer;
    size_t pointer;
    size_t length;
} yaml_istring_t;

/*
 * A string that is used as an output buffer.
 */

typedef struct yaml_ostring_s {
    yaml_char_t *buffer;
    size_t pointer;
    size_t capacity;
} yaml_ostring_t;

/*
 * A string that could be used both as an input and an output buffer.
 */

typedef struct yaml_iostring_s {
    yaml_char_t *buffer;
    size_t pointer;
    size_t length;
    size_t capacity;
} yaml_iostring_t;

/*
 * A separate type for non-UTF-8 i/o strings.
 */

typedef struct yaml_raw_iostring_s {
    unsigned char *buffer;
    size_t pointer;
    size_t length;
    size_t capacity;
} yaml_raw_iostring_t;

/*
 * Double the string capacity.
 */

YAML_DECLARE(int)
yaml_ostring_extend(yaml_char_t **buffer, size_t *capacity);

/*
 * Append a string to the end of the base string expanding it if needed.
 */

YAML_DECLARE(int)
yaml_ostring_join(
        yaml_char_t **base_buffer, size_t *base_pointer, size_t *base_capacity,
        yaml_char_t *adj_buffer, size_t adj_pointer);

/*
 * Basic string operations.
 */

#define ISTRING(buffer, length) { (buffer), 0, (length) }

#define NULL_OSTRING { NULL, 0, 0 }

#define IOSTRING_INIT(self, string, _capacity)                                  \
    (((string).buffer = yaml_malloc(_capacity)) ?                               \
        ((string).pointer = (string).length = 0,                                \
         (string).capacity = (_capacity),                                       \
         memset((string).buffer, 0, (_capacity)),                               \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

#define IOSTRING_DEL(self, string)                                              \
    (yaml_free((string).buffer),                                                \
     (string).buffer = NULL,                                                    \
     ((string).pointer = (string).length = (string).capacity = 0))

#define IOSTRING_SET(self, string, _buffer, _capacity)                          \
    (memset((_buffer), 0, (_capacity)),                                         \
     (string).buffer = (_buffer),                                               \
     (string).pointer = (string).length = 0,                                    \
     (string).capacity = (_capacity))

#define OSTRING_INIT(self, string, _capacity)                                   \
    (((string).buffer = yaml_malloc(_capacity)) ?                               \
        ((string).pointer = 0,                                                  \
         (string).capacity = (_capacity),                                       \
         memset((string).buffer, 0, (_capacity)),                               \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

#define OSTRING_DEL(self, string)                                               \
    (yaml_free((string).buffer),                                                \
     (string).buffer = NULL,                                                    \
     ((string).pointer = (string).capacity = 0))

#define OSTRING_EXTEND(self, string)                                            \
    ((((string).pointer+5 < (string).capacity)                                  \
        || yaml_ostring_extend(&(string).buffer, &(string).capacity)) ?         \
     1 :                                                                        \
     ((self)->error.type = YAML_MEMORY_ERROR,                                   \
      0))

#define CLEAR(self, string)                                                     \
    ((string).pointer = 0,                                                      \
     memset((string).buffer, 0, (string).capacity))

#define JOIN(self, base_string, adj_string)                                     \
    ((yaml_ostring_join(&(base_string).buffer, &(base_string).pointer,          \
                       &(base_string).capacity,                                 \
                       (adj_string).buffer, (adj_string).pointer)) ?            \
        ((adj_string).pointer = 0,                                              \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

/*****************************************************************************
 * String Tests
 *****************************************************************************/

/*
 * Get the octet at the specified position.
 */

#define OCTET_AT(string, offset)                                                \
    ((string).buffer[(string).pointer+(offset)])

/*
 * Get the current offset.
 */

#define OCTET(string)   OCTET_AT((string), 0)

/*
 * Check the octet at the specified position.
 */

#define CHECK_AT(string, octet, offset)                                         \
    (OCTET_AT((string), (offset)) == (yaml_char_t)(octet))

/*
 * Check the current octet in the buffer.
 */

#define CHECK(string, octet)    CHECK_AT((string), (octet), 0)

/*
 * Check if the character at the specified position is an alphabetical
 * character, a digit, '_', or '-'.
 */

#define IS_ALPHA_AT(string, offset)                                             \
     ((OCTET_AT((string), (offset)) >= (yaml_char_t) '0' &&                     \
       OCTET_AT((string), (offset)) <= (yaml_char_t) '9') ||                    \
      (OCTET_AT((string), (offset)) >= (yaml_char_t) 'A' &&                     \
       OCTET_AT((string), (offset)) <= (yaml_char_t) 'Z') ||                    \
      (OCTET_AT((string), (offset)) >= (yaml_char_t) 'a' &&                     \
       OCTET_AT((string), (offset)) <= (yaml_char_t) 'z') ||                    \
      OCTET_AT((string), (offset)) == '_' ||                                    \
      OCTET_AT((string), (offset)) == '-')

#define IS_ALPHA(string)    IS_ALPHA_AT((string), 0)

/*
 * Check if the character at the specified position is a digit.
 */

#define IS_DIGIT_AT(string, offset)                                             \
     ((OCTET_AT((string), (offset)) >= (yaml_char_t) '0' &&                     \
       OCTET_AT((string), (offset)) <= (yaml_char_t) '9'))

#define IS_DIGIT(string)    IS_DIGIT_AT((string), 0)

/*
 * Get the value of a digit.
 */

#define AS_DIGIT_AT(string, offset)                                             \
     (OCTET_AT((string), (offset)) - (yaml_char_t) '0')

#define AS_DIGIT(string)    AS_DIGIT_AT((string), 0)

/*
 * Check if the character at the specified position is a hex-digit.
 */

#define IS_HEX_AT(string, offset)                                               \
     ((OCTET_AT((string), (offset)) >= (yaml_char_t) '0' &&                     \
       OCTET_AT((string), (offset)) <= (yaml_char_t) '9') ||                    \
      (OCTET_AT((string), (offset)) >= (yaml_char_t) 'A' &&                     \
       OCTET_AT((string), (offset)) <= (yaml_char_t) 'F') ||                    \
      (OCTET_AT((string), (offset)) >= (yaml_char_t) 'a' &&                     \
       OCTET_AT((string), (offset)) <= (yaml_char_t) 'f'))

#define IS_HEX(string)    IS_HEX_AT((string), 0)

/*
 * Get the value of a hex-digit.
 */

#define AS_HEX_AT(string, offset)                                               \
      ((OCTET_AT((string), (offset)) >= (yaml_char_t) 'A' &&                    \
        OCTET_AT((string), (offset)) <= (yaml_char_t) 'F') ?                    \
       (OCTET_AT((string), (offset)) - (yaml_char_t) 'A' + 10) :                \
       (OCTET_AT((string), (offset)) >= (yaml_char_t) 'a' &&                    \
        OCTET_AT((string), (offset)) <= (yaml_char_t) 'f') ?                    \
       (OCTET_AT((string), (offset)) - (yaml_char_t) 'a' + 10) :                \
       (OCTET_AT((string), (offset)) - (yaml_char_t) '0'))
 
#define AS_HEX(string)  AS_HEX_AT((string), 0)
 
/*
 * Check if the character is ASCII.
 */

#define IS_ASCII_AT(string, offset)                                             \
    (OCTET_AT((string), (offset)) <= (yaml_char_t) '\x7F')

#define IS_ASCII(string)    IS_ASCII_AT((string), 0)

/*
 * Check if the character can be printed unescaped.
 */

#define IS_PRINTABLE_AT(string, offset)                                         \
    ((OCTET_AT((string), (offset)) == 0x0A)         /* . == #x0A */             \
     || (OCTET_AT((string), (offset)) >= 0x20       /* #x20 <= . <= #x7E */     \
         && OCTET_AT((string), (offset)) <= 0x7E)                               \
     || (OCTET_AT((string), (offset)) == 0xC2       /* #0xA0 <= . <= #xD7FF */  \
         && OCTET_AT((string), (offset)+1) >= 0xA0)                             \
     || (OCTET_AT((string), (offset)) > 0xC2                                    \
         && OCTET_AT((string), (offset)) < 0xED)                                \
     || (OCTET_AT((string), (offset)) == 0xED                                   \
         && OCTET_AT((string), (offset)+1) < 0xA0)                              \
     || (OCTET_AT((string), (offset)) == 0xEE)                                  \
     || (OCTET_AT((string), (offset)) == 0xEF       /* #xE000 <= . <= #xFFFD */ \
         && !(OCTET_AT((string), (offset)+1) == 0xBB       /* && . != #xFEFF */ \
             && OCTET_AT((string), (offset)+2) == 0xBF)                         \
         && !(OCTET_AT((string), (offset)+1) == 0xBF                            \
             && (OCTET_AT((string), (offset)+2) == 0xBE                         \
                 || OCTET_AT((string), (offset)+2) == 0xBF))))

#define IS_PRINTABLE(string)    IS_PRINTABLE_AT((string), 0)

/*
 * Check if the character at the specified position is NUL.
 */

#define IS_Z_AT(string, offset)   CHECK_AT((string), '\0', (offset))

#define IS_Z(string)    IS_Z_AT((string), 0)

/*
 * Check if the character at the specified position is BOM.
 */

#define IS_BOM_AT(string, offset)                                               \
     (CHECK_AT((string), '\xEF', (offset))                                      \
      && CHECK_AT((string), '\xBB', (offset)+1)                                 \
      && CHECK_AT((string), '\xBF', (offset)+2))    /* BOM (#xFEFF) */

#define IS_BOM(string)  IS_BOM_AT(string, 0)

/*
 * Check if the character at the specified position is space.
 */

#define IS_SPACE_AT(string, offset) CHECK_AT((string), ' ', (offset))

#define IS_SPACE(string)    IS_SPACE_AT((string), 0)

/*
 * Check if the character at the specified position is tab.
 */

#define IS_TAB_AT(string, offset)   CHECK_AT((string), '\t', (offset))

#define IS_TAB(string)  IS_TAB_AT((string), 0)

/*
 * Check if the character at the specified position is blank (space or tab).
 */

#define IS_BLANK_AT(string, offset)                                             \
    (IS_SPACE_AT((string), (offset)) || IS_TAB_AT((string), (offset)))

#define IS_BLANK(string)    IS_BLANK_AT((string), 0)

/*
 * Check if the character at the specified position is a line break.
 */

#define IS_BREAK_AT(string, offset)                                             \
    (CHECK_AT((string), '\r', (offset))                 /* CR (#xD)*/           \
     || CHECK_AT((string), '\n', (offset))              /* LF (#xA) */          \
     || (CHECK_AT((string), '\xC2', (offset))                                   \
         && CHECK_AT((string), '\x85', (offset)+1))     /* NEL (#x85) */        \
     || (CHECK_AT((string), '\xE2', (offset))                                   \
         && CHECK_AT((string), '\x80', (offset)+1)                              \
         && CHECK_AT((string), '\xA8', (offset)+2))     /* LS (#x2028) */       \
     || (CHECK_AT((string), '\xE2', (offset))                                   \
         && CHECK_AT((string), '\x80', (offset)+1)                              \
         && CHECK_AT((string), '\xA9', (offset)+2)))    /* PS (#x2029) */

#define IS_BREAK(string)    IS_BREAK_AT((string), 0)

#define IS_CRLF_AT(string, offset)                                              \
     (CHECK_AT((string), '\r', (offset)) && CHECK_AT((string), '\n', (offset)+1))

#define IS_CRLF(string) IS_CRLF_AT((string), 0)

/*
 * Check if the character is a line break or NUL.
 */

#define IS_BREAKZ_AT(string, offset)                                            \
    (IS_BREAK_AT((string), (offset)) || IS_Z_AT((string), (offset)))

#define IS_BREAKZ(string)   IS_BREAKZ_AT((string), 0)

/*
 * Check if the character is a line break, space, or NUL.
 */

#define IS_SPACEZ_AT(string, offset)                                            \
    (IS_SPACE_AT((string), (offset)) || IS_BREAKZ_AT((string), (offset)))

#define IS_SPACEZ(string)   IS_SPACEZ_AT((string), 0)

/*
 * Check if the character is a line break, space, tab, or NUL.
 */

#define IS_BLANKZ_AT(string, offset)                                            \
    (IS_BLANK_AT((string), (offset)) || IS_BREAKZ_AT((string), (offset)))

#define IS_BLANKZ(string)   IS_BLANKZ_AT((string), 0)

/*
 * Determine the width of the character.
 */

#define WIDTH_AT(string, offset)                                                \
     ((OCTET_AT((string), (offset)) & 0x80) == 0x00 ? 1 :                       \
      (OCTET_AT((string), (offset)) & 0xE0) == 0xC0 ? 2 :                       \
      (OCTET_AT((string), (offset)) & 0xF0) == 0xE0 ? 3 :                       \
      (OCTET_AT((string), (offset)) & 0xF8) == 0xF0 ? 4 : 0)

#define WIDTH(string)   WIDTH_AT((string), 0)

/*
 * Move the string pointer to the next character.
 */

#define MOVE(string)    ((string).pointer += WIDTH((string)))

/*
 * Write a single octet and bump the pointer.
 */

#define JOIN_OCTET(string, octet)                                               \
    ((string).buffer[(string).pointer++] = (octet))

/*
 * Copy a single octet and bump the pointers.
 */

#define COPY_OCTET(target_string, source_string)                                \
    ((target_string).buffer[(target_string).pointer++]                          \
     = (source_string).buffer[(source_string).pointer++])

/*
 * Copy a character and move the pointers of both strings.
 */

#define COPY(target_string, source_string)                                      \
    ((OCTET(source_string) & 0x80) == 0x00 ?                                    \
     COPY_OCTET((target_string), (source_string)) :                             \
     (OCTET(source_string) & 0xE0) == 0xC0 ?                                    \
     (COPY_OCTET((target_string), (source_string)),                             \
      COPY_OCTET((target_string), (source_string))) :                           \
     (OCTET(source_string) & 0xF0) == 0xE0 ?                                    \
     (COPY_OCTET((target_string), (source_string)),                             \
      COPY_OCTET((target_string), (source_string)),                             \
      COPY_OCTET((target_string), (source_string))) :                           \
     (OCTET(source_string) & 0xF8) == 0xF0 ?                                    \
     (COPY_OCTET((target_string), (source_string)),                             \
      COPY_OCTET((target_string), (source_string)),                             \
      COPY_OCTET((target_string), (source_string)),                             \
      COPY_OCTET((target_string), (source_string))) : 0)

/*****************************************************************************
 * Stack and Queue Management
 *****************************************************************************/

/*
 * Double the stack capacity.
 */

YAML_DECLARE(int)
yaml_stack_extend(void **list, size_t size, size_t *length, size_t *capacity);

/*
 * Double the queue capacity.
 */

YAML_DECLARE(int)
yaml_queue_extend(void **list, size_t size,
        size_t *head, size_t *tail, size_t *capacity);

/*
 * Basic stack operations.
 */

#define STACK_INIT(self, stack, _capacity)                                      \
    (((stack).list = yaml_malloc((_capacity)*sizeof(*(stack).list))) ?          \
        ((stack).length = 0,                                                    \
         (stack).capacity = (_capacity),                                        \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

#define STACK_DEL(self, stack)                                                  \
    (yaml_free((stack).list),                                                   \
     (stack).list = NULL,                                                       \
     (stack).length = (stack).capacity = 0)

#define STACK_SET(self, stack, _list, _capacity)                                \
    ((stack).list = (_list),                                                    \
     (stack).length = 0,                                                        \
     (stack).capacity = (_capacity))

#define STACK_EMPTY(self, stack)                                                \
    ((stack).length == 0)

#define STACK_ITER(self, stack, index)                                          \
    ((stack).list + index)

#define PUSH(self, stack, value)                                                \
    (((stack).length < (stack).capacity                                         \
      || yaml_stack_extend((void **)&(stack).list, sizeof(*(stack).list),       \
              &(stack).length, &(stack).capacity)) ?                            \
        ((stack).list[(stack).length++] = (value),                              \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

#define POP(self, stack)                                                        \
    ((stack).list[--(stack).length])

/*
 * Basic queue operations.
 */

#define QUEUE_INIT(self, queue, _capacity)                                      \
    (((queue).list = yaml_malloc((_capacity)*sizeof(*(queue).list))) ?          \
        ((queue).head = (queue).tail = 0,                                       \
         (queue).capacity = (_capacity),                                        \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

#define QUEUE_DEL(self, queue)                                                  \
    (yaml_free((queue).list),                                                   \
     (queue).list = NULL,                                                       \
     (queue).head = (queue).tail = (queue).capacity = 0)

#define QUEUE_SET(self, queue, _list, _capacity)                                \
    ((queue).list = (_list),                                                    \
     (queue).head = (queue).tail = 0,                                           \
     (queue).capacity = (_capacity))

#define QUEUE_EMPTY(self, queue)                                                \
    ((queue).head == (queue).tail)

#define QUEUE_ITER(self, queue, index)                                          \
    ((queue).list + (queue).head + index)

#define ENQUEUE(self, queue, value)                                             \
    (((queue).tail != (queue).capacity                                          \
      || yaml_queue_extend((void **)&(queue).list, sizeof(*(queue).list),       \
          &(queue).head, &(queue).tail, &(queue).capacity)) ?                   \
        ((queue).list[(queue).tail++] = (value),                                \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

#define DEQUEUE(self, queue)                                                    \
    ((queue).list[(queue).head++])

#define QUEUE_INSERT(self, queue, index, value)                                 \
    (((queue).tail != (queue).capacity                                          \
      || yaml_queue_extend((void **)&(queue).list, sizeof(*(queue).list),       \
          &(queue).head, &(queue).tail, &(queue).capacity)) ?                   \
        (memmove((queue).list+(queue).head+(index)+1,                           \
                 (queue).list+(queue).head+(index),                             \
            ((queue).tail-(queue).head-(index))*sizeof(*(queue).list)),         \
         (queue).list[(queue).head+(index)] = (value),                          \
         (queue).tail++,                                                        \
         1) :                                                                   \
        ((self)->error.type = YAML_MEMORY_ERROR,                                \
         0))

/*****************************************************************************
 * Token Initializers
 *****************************************************************************/

#define TOKEN_INIT(token, _type, _start_mark, _end_mark)                        \
    (memset(&(token), 0, sizeof(yaml_token_t)),                                 \
     (token).type = (_type),                                                    \
     (token).start_mark = (_start_mark),                                        \
     (token).end_mark = (_end_mark))

#define STREAM_START_TOKEN_INIT(token, _encoding, _start_mark, _end_mark)       \
    (TOKEN_INIT((token), YAML_STREAM_START_TOKEN, (_start_mark), (_end_mark)),  \
     (token).data.stream_start.encoding = (_encoding))

#define STREAM_END_TOKEN_INIT(token, _start_mark, _end_mark)                    \
    (TOKEN_INIT((token), YAML_STREAM_END_TOKEN, (_start_mark), (_end_mark)))

#define ALIAS_TOKEN_INIT(token, _value, _start_mark, _end_mark)                 \
    (TOKEN_INIT((token), YAML_ALIAS_TOKEN, (_start_mark), (_end_mark)),         \
     (token).data.alias.value = (_value))

#define ANCHOR_TOKEN_INIT(token, _value, _start_mark, _end_mark)                \
    (TOKEN_INIT((token), YAML_ANCHOR_TOKEN, (_start_mark), (_end_mark)),        \
     (token).data.anchor.value = (_value))

#define TAG_TOKEN_INIT(token, _handle, _suffix, _start_mark, _end_mark)         \
    (TOKEN_INIT((token), YAML_TAG_TOKEN, (_start_mark), (_end_mark)),           \
     (token).data.tag.handle = (_handle),                                       \
     (token).data.tag.suffix = (_suffix))

#define SCALAR_TOKEN_INIT(token, _value, _length, _style, _start_mark, _end_mark)   \
    (TOKEN_INIT((token), YAML_SCALAR_TOKEN, (_start_mark), (_end_mark)),        \
     (token).data.scalar.value = (_value),                                      \
     (token).data.scalar.length = (_length),                                    \
     (token).data.scalar.style = (_style))

#define VERSION_DIRECTIVE_TOKEN_INIT(token, _major, _minor, _start_mark, _end_mark) \
    (TOKEN_INIT((token), YAML_VERSION_DIRECTIVE_TOKEN, (_start_mark), (_end_mark)), \
     (token).data.version_directive.major = (_major),                           \
     (token).data.version_directive.minor = (_minor))

#define TAG_DIRECTIVE_TOKEN_INIT(token, _handle, _prefix, _start_mark, _end_mark)   \
    (TOKEN_INIT((token), YAML_TAG_DIRECTIVE_TOKEN, (_start_mark), (_end_mark)), \
     (token).data.tag_directive.handle = (_handle),                             \
     (token).data.tag_directive.prefix = (_prefix))

/*****************************************************************************
 * Event Initializers
 *****************************************************************************/

#define EVENT_INIT(event, _type, _start_mark, _end_mark)                        \
    (memset(&(event), 0, sizeof(yaml_event_t)),                                 \
     (event).type = (_type),                                                    \
     (event).start_mark = (_start_mark),                                        \
     (event).end_mark = (_end_mark))

#define STREAM_START_EVENT_INIT(event, _encoding, _start_mark, _end_mark)       \
    (EVENT_INIT((event), YAML_STREAM_START_EVENT, (_start_mark), (_end_mark)),  \
     (event).data.stream_start.encoding = (_encoding))

#define STREAM_END_EVENT_INIT(event, _start_mark, _end_mark)                    \
    (EVENT_INIT((event), YAML_STREAM_END_EVENT, (_start_mark), (_end_mark)))

#define DOCUMENT_START_EVENT_INIT(event, _version_directive,                    \
        _tag_directives_list, _tag_directives_length, _tag_directives_capacity, \
        _is_implicit, _start_mark, _end_mark)                                   \
    (EVENT_INIT((event), YAML_DOCUMENT_START_EVENT, (_start_mark),(_end_mark)), \
     (event).data.document_start.version_directive = (_version_directive),      \
     (event).data.document_start.tag_directives.list = (_tag_directives_list),  \
     (event).data.document_start.tag_directives.length = (_tag_directives_length),  \
     (event).data.document_start.tag_directives.capacity = (_tag_directives_capacity),  \
     (event).data.document_start.is_implicit = (_is_implicit))

#define DOCUMENT_END_EVENT_INIT(event, _is_implicit, _start_mark, _end_mark)    \
    (EVENT_INIT((event), YAML_DOCUMENT_END_EVENT, (_start_mark), (_end_mark)),  \
     (event).data.document_end.is_implicit = (_is_implicit))

#define ALIAS_EVENT_INIT(event, _anchor, _start_mark, _end_mark)                \
    (EVENT_INIT((event), YAML_ALIAS_EVENT, (_start_mark), (_end_mark)),         \
     (event).data.alias.anchor = (_anchor))

#define SCALAR_EVENT_INIT(event, _anchor, _tag, _value, _length,                \
        _is_plain_nonspecific, _is_quoted_nonspecific, _style, _start_mark, _end_mark)  \
    (EVENT_INIT((event), YAML_SCALAR_EVENT, (_start_mark), (_end_mark)),        \
     (event).data.scalar.anchor = (_anchor),                                    \
     (event).data.scalar.tag = (_tag),                                          \
     (event).data.scalar.value = (_value),                                      \
     (event).data.scalar.length = (_length),                                    \
     (event).data.scalar.is_plain_nonspecific = (_is_plain_nonspecific),        \
     (event).data.scalar.is_quoted_nonspecific = (_is_quoted_nonspecific),      \
     (event).data.scalar.style = (_style))

#define SEQUENCE_START_EVENT_INIT(event, _anchor, _tag, _is_nonspecific, _style,    \
        _start_mark, _end_mark)                                                 \
    (EVENT_INIT((event), YAML_SEQUENCE_START_EVENT, (_start_mark), (_end_mark)),    \
     (event).data.sequence_start.anchor = (_anchor),                            \
     (event).data.sequence_start.tag = (_tag),                                  \
     (event).data.sequence_start.is_nonspecific = (_is_nonspecific),            \
     (event).data.sequence_start.style = (_style))

#define SEQUENCE_END_EVENT_INIT(event, _start_mark, _end_mark)                  \
    (EVENT_INIT((event), YAML_SEQUENCE_END_EVENT, (_start_mark), (_end_mark)))

#define MAPPING_START_EVENT_INIT(event, _anchor, _tag, _is_nonspecific, _style, \
        _start_mark, _end_mark)                                                 \
    (EVENT_INIT((event), YAML_MAPPING_START_EVENT, (_start_mark), (_end_mark)), \
     (event).data.mapping_start.anchor = (_anchor),                             \
     (event).data.mapping_start.tag = (_tag),                                   \
     (event).data.mapping_start.is_nonspecific = (_is_nonspecific),             \
     (event).data.mapping_start.style = (_style))

#define MAPPING_END_EVENT_INIT(event, _start_mark, _end_mark)                   \
    (EVENT_INIT((event), YAML_MAPPING_END_EVENT, (_start_mark), (_end_mark)))

/*****************************************************************************
 * Document, Node and Arc Initializers
 *****************************************************************************/

#define DOCUMENT_INIT(document, _nodes_list, _nodes_length, _nodes_capacity,    \
        _version_directive, _tag_directives_list, _tag_directives_length,       \
        _tag_directives_capacity, _is_start_implicit, _is_end_implicit,         \
        _start_mark, _end_mark)                                                 \
    (memset(&(document), 0, sizeof(yaml_document_t)),                           \
     (document).type = YAML_DOCUMENT,                                           \
     (document).nodes.list = (_nodes_list),                                     \
     (document).nodes.length = (_nodes_length),                                 \
     (document).nodes.capacity = (_nodes_capacity),                             \
     (document).version_directive = (_version_directive),                       \
     (document).tag_directives.list = (_tag_directives_list),                   \
     (document).tag_directives.length = (_tag_directives_length),               \
     (document).tag_directives.capacity = (_tag_directives_capacity),           \
     (document).is_start_implicit = (_is_start_implicit),                       \
     (document).is_end_implicit = (_is_end_implicit),                           \
     (document).start_mark = (_start_mark),                                     \
     (document).end_mark = (_end_mark))

#define NODE_INIT(node, _type, _anchor, _tag, _start_mark, _end_mark)           \
    (memset(&(node), 0, sizeof(yaml_node_t)),                                   \
     (node).type = (_type),                                                     \
     (node).anchor = (_anchor),                                                 \
     (node).tag = (_tag),                                                       \
     (node).start_mark = (_start_mark),                                         \
     (node).end_mark = (_end_mark))

#define SCALAR_NODE_INIT(node, _anchor, _tag, _value, _length, _style,          \
        _start_mark, _end_mark)                                                 \
    (NODE_INIT((node), YAML_SCALAR_NODE, (_anchor), (_tag),                     \
               (_start_mark), (_end_mark)),                                     \
     (node).data.scalar.value = (_value),                                       \
     (node).data.scalar.length = (_length),                                     \
     (node).data.scalar.style = (_style))

#define SEQUENCE_NODE_INIT(node, _anchor, _tag, _items_list, _items_length,     \
        _items_capacity, _style, _start_mark, _end_mark)                        \
    (NODE_INIT((node), YAML_SEQUENCE_NODE, (_anchor), (_tag),                   \
               (_start_mark), (_end_mark)),                                     \
     (node).data.sequence.items.list = (_items_list),                           \
     (node).data.sequence.items.length = (_items_length),                       \
     (node).data.sequence.items.capacity = (_items_capacity),                   \
     (node).data.sequence.style = (_style))

#define MAPPING_NODE_INIT(node, _anchor, _tag, _pairs_list, _pairs_length,      \
        _pairs_capacity, _style, _start_mark, _end_mark)                        \
    (NODE_INIT((node), YAML_MAPPING_NODE, (_anchor), (_tag),                    \
               (_start_mark), (_end_mark)),                                     \
     (node).data.mapping.pairs.list = (_pairs_list),                            \
     (node).data.mapping.pairs.length = (_pairs_length),                        \
     (node).data.mapping.pairs.capacity = (_pairs_capacity),                    \
     (node).data.mapping.style = (_style))

#define ARC_INIT(arc, _type, _tag)                                              \
    (memset(&(arc), 0, sizeof(yaml_arc_t)),                                     \
     (arc).type = (_type),                                                      \
     (arc).tag = (_tag))

#define SEQUENCE_ITEM_ARC_INIT(arc, _tag, _index)                               \
    (ARC_INIT((arc), YAML_SEQUENCE_ITEM_ARC, (_tag)),                           \
     (arc).data.item.index = (_index))

#define MAPPING_KEY_ARC_INIT(arc, _tag)                                         \
    ARC_INIT((arc), YAML_MAPPING_KEY_ARC, (_tag))

#define MAPPING_VALUE_ARC_INIT(arc, _tag, _key_type, _key_tag)                  \
    (ARC_INIT((arc), YAML_MAPPING_VALUE_ARC, (_tag)),                           \
     (arc).data.value.key.type = (_key_type),                                   \
     (arc).data.value.key.tag = (_key_tag))

#define MAPPING_VALUE_FOR_SCALAR_KEY_ARC_INIT(arc, _tag, _key_tag,              \
        _key_value, _key_length)                                                \
    (MAPPING_VALUE_ARC_INIT((arc), (_tag), YAML_SCALAR_NODE, (_key_tag)),       \
     (arc).data.value.key.data.scalar.value = (_key_value),                     \
     (arc).data.value.key.data.scalar.length = (_key_length))

#define MAPPING_VALUE_FOR_SEQUENCE_KEY_ARC_INIT(arc, _tag, _key_tag)            \
    MAPPING_VALUE_ARC_INIT((arc), (_tag), YAML_SEQUENCE_NODE, (_key_tag))

#define MAPPING_VALUE_FOR_MAPPING_KEY_INIT(arc, _tag, _key_tag)                 \
    MAPPING_VALUE_ARC_INIT((arc), (_tag), YAML_MAPPING_NODE, (_key_tag))

#define INCOMPLETE_NODE_INIT(node, _type, _path_list, _path_length,             \
        _path_capacity, _mark)                                                  \
    (memset(&(node), 0, sizeof(yaml_incomplete_node_t)),                        \
     (node).type = (_type),                                                     \
     (node).path.list = (_path_list),                                           \
     (node).path.length = (_path_length),                                       \
     (node).path.capacity = (_path_capacity),                                   \
     (node).mark = (_mark))

#define INCOMPLETE_SCALAR_NODE_INIT(node, _path_list, _path_length,             \
        _path_capacity, _value, _length, _is_plain, _mark)                      \
    (INCOMPLETE_NODE_INIT((node), YAML_SCALAR_NODE, (_path_list),               \
                          (_path_length), (_path_capacity), (_mark)).           \
     (node).data.scalar.value = (_value),                                       \
     (node).data.scalar.length = (_length),                                     \
     (node).data.scalar.is_plain = (_is_plain))

#define INCOMPLETE_SEQUENCE_NODE_INIT(node, _path_list, _path_length,           \
        _path_capacity, _mark)                                                  \
    INCOMPLETE_NODE_INIT((node), YAML_SEQUENCE_NODE, (_path_list),              \
            (_path_length), (_path_capacity), (_mark))

#define INCOMPLETE_MAPPING_NODE_INIT(node, _path_list, _path_length,            \
        _path_capacity, _mark)                                                  \
    INCOMPLETE_NODE_INIT((node), YAML_MAPPING_NODE, (_path_list),               \
            (_path_length), (_path_capacity), (_mark))

/*****************************************************************************
 * Parser Structures
 *****************************************************************************/

/*
 * This structure holds information about a potential simple key.
 */

typedef struct yaml_simple_key_s {
    /* Is a simple key possible? */
    int is_possible;
    /* Is a simple key required? */
    int is_required;
    /* The number of the token. */
    size_t token_number;
    /* The position mark. */
    yaml_mark_t mark;
} yaml_simple_key_t;

/*
 * The states of the parser.
 */

typedef enum yaml_parser_state_e {
    /* Expect STREAM-START. */
    YAML_PARSE_STREAM_START_STATE,
    /* Expect the beginning of an implicit document. */
    YAML_PARSE_IMPLICIT_DOCUMENT_START_STATE,
    /* Expect DOCUMENT-START. */
    YAML_PARSE_DOCUMENT_START_STATE,
    /* Expect the content of a document. */
    YAML_PARSE_DOCUMENT_CONTENT_STATE,
    /* Expect DOCUMENT-END. */
    YAML_PARSE_DOCUMENT_END_STATE,
    /* Expect a block node. */
    YAML_PARSE_BLOCK_NODE_STATE,
    /* Expect a block node or indentless sequence. */
    YAML_PARSE_BLOCK_NODE_OR_INDENTLESS_SEQUENCE_STATE,
    /* Expect a flow node. */
    YAML_PARSE_FLOW_NODE_STATE,
    /* Expect the first entry of a block sequence. */
    YAML_PARSE_BLOCK_SEQUENCE_FIRST_ENTRY_STATE,
    /* Expect an entry of a block sequence. */
    YAML_PARSE_BLOCK_SEQUENCE_ENTRY_STATE,
    /* Expect an entry of an indentless sequence. */
    YAML_PARSE_INDENTLESS_SEQUENCE_ENTRY_STATE,
    /* Expect the first key of a block mapping. */
    YAML_PARSE_BLOCK_MAPPING_FIRST_KEY_STATE,
    /* Expect a block mapping key. */
    YAML_PARSE_BLOCK_MAPPING_KEY_STATE,
    /* Expect a block mapping value. */
    YAML_PARSE_BLOCK_MAPPING_VALUE_STATE,
    /* Expect the first entry of a flow sequence. */
    YAML_PARSE_FLOW_SEQUENCE_FIRST_ENTRY_STATE,
    /* Expect an entry of a flow sequence. */
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_STATE,
    /* Expect a key of an ordered mapping. */
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_KEY_STATE,
    /* Expect a value of an ordered mapping. */
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_VALUE_STATE,
    /* Expect the and of an ordered mapping entry. */
    YAML_PARSE_FLOW_SEQUENCE_ENTRY_MAPPING_END_STATE,
    /* Expect the first key of a flow mapping. */
    YAML_PARSE_FLOW_MAPPING_FIRST_KEY_STATE,
    /* Expect a key of a flow mapping. */
    YAML_PARSE_FLOW_MAPPING_KEY_STATE,
    /* Expect a value of a flow mapping. */
    YAML_PARSE_FLOW_MAPPING_VALUE_STATE,
    /* Expect an empty value of a flow mapping. */
    YAML_PARSE_FLOW_MAPPING_EMPTY_VALUE_STATE,
    /* Expect nothing. */
    YAML_PARSE_END_STATE
} yaml_parser_state_t;

/*
 * This structure holds aliases data.
 */

typedef struct yaml_alias_data_s {
    /* The anchor. */
    yaml_char_t *anchor;
    /* The node id. */
    int index;
    /* The anchor mark. */
    yaml_mark_t mark;
} yaml_alias_data_t;

/*
 * The structure that holds data used by the file and string readers.
 */

typedef struct yaml_standard_reader_data_t {
    /* String input data. */
    yaml_istring_t string;
    /* File input data. */
    FILE *file;
} yaml_standard_reader_data_t;

/*
 * The internal parser structure.
 */

struct yaml_parser_s {

    /*
     * Error stuff.
     */

    yaml_error_t error;

    /*
     * Reader stuff.
     */

    /* The read handler. */
    yaml_reader_t *reader;

    /* The application data to be passed to the reader. */
    void *reader_data;

    /* Standard (string or file) input data. */
    yaml_standard_reader_data_t standard_reader_data;

    /* EOF flag. */
    int is_eof;

    /* The working buffer. */
    yaml_iostring_t input;

    /* The number of unread characters in the buffer. */
    size_t unread;

    /* The raw buffer. */
    yaml_raw_iostring_t raw_input;

    /* The input encoding. */
    yaml_encoding_t encoding;

    /* The offset of the current position (in bytes). */
    size_t offset;

    /* The mark of the current position. */
    yaml_mark_t mark;

    /*
     * Scanner stuff.
     */

    /* Have we started to scan the input stream? */
    int is_stream_start_produced;

    /* Have we reached the end of the input stream? */
    int is_stream_end_produced;

    /* The number of unclosed '[' and '{' indicators. */
    int flow_level;

    /* The tokens queue. */
    struct {
        yaml_token_t *list;
        size_t head;
        size_t tail;
        size_t capacity;
    } tokens;

    /* The number of tokens fetched from the queue. */
    size_t tokens_parsed;

    /* Does the tokens queue contain a token ready for dequeueing. */
    int is_token_available;

    /* The indentation levels stack. */
    struct {
        int *list;
        size_t length;
        size_t capacity;
    } indents;

    /* The current indentation level. */
    int indent;

    /* May a simple key occur at the current position? */
    int is_simple_key_allowed;

    /* The stack of simple keys. */
    struct {
        yaml_simple_key_t *list;
        size_t length;
        size_t capacity;
    } simple_keys;

    /*
     * Parser stuff.
     */

    /* The parser states stack. */
    struct {
        yaml_parser_state_t *list;
        size_t length;
        size_t capacity;
    } states;

    /* The current parser state. */
    yaml_parser_state_t state;

    /* The stack of marks. */
    struct {
        yaml_mark_t *list;
        size_t length;
        size_t capacity;
    } marks;

    /* The list of TAG directives. */
    struct {
        yaml_tag_directive_t *list;
        size_t length;
        size_t capacity;
    } tag_directives;

    /*
     * Dumper stuff.
     */

    /* The resolve handler. */
    yaml_resolver_t *resolver;

    /* The application data to be passed to the resolver. */
    void *resolver_data;

    /* The alias data. */
    struct {
        yaml_alias_data_t *list;
        size_t length;
        size_t capacity;
    } aliases;

    /* The document being parsed. */
    yaml_document_t *document;

};

/*****************************************************************************
 * Internal Parser API
 *****************************************************************************/

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

/*****************************************************************************
 * Emitter Structures
 *****************************************************************************/

/*
 * The emitter states.
 */

typedef enum yaml_emitter_state_e {
    /* Expect STREAM-START. */
    YAML_EMIT_STREAM_START_STATE,
    /* Expect the first DOCUMENT-START or STREAM-END. */
    YAML_EMIT_FIRST_DOCUMENT_START_STATE,
    /* Expect DOCUMENT-START or STREAM-END. */
    YAML_EMIT_DOCUMENT_START_STATE,
    /* Expect the content of a document. */
    YAML_EMIT_DOCUMENT_CONTENT_STATE,
    /* Expect DOCUMENT-END. */
    YAML_EMIT_DOCUMENT_END_STATE,
    /* Expect the first item of a flow sequence. */
    YAML_EMIT_FLOW_SEQUENCE_FIRST_ITEM_STATE,
    /* Expect an item of a flow sequence. */
    YAML_EMIT_FLOW_SEQUENCE_ITEM_STATE,
    /* Expect the first key of a flow mapping. */
    YAML_EMIT_FLOW_MAPPING_FIRST_KEY_STATE,
    /* Expect a key of a flow mapping. */
    YAML_EMIT_FLOW_MAPPING_KEY_STATE,
    /* Expect a value for a simple key of a flow mapping. */
    YAML_EMIT_FLOW_MAPPING_SIMPLE_VALUE_STATE,
    /* Expect a value of a flow mapping. */
    YAML_EMIT_FLOW_MAPPING_VALUE_STATE,
    /* Expect the first item of a block sequence. */
    YAML_EMIT_BLOCK_SEQUENCE_FIRST_ITEM_STATE,
    /* Expect an item of a block sequence. */
    YAML_EMIT_BLOCK_SEQUENCE_ITEM_STATE,
    /* Expect the first key of a block mapping. */
    YAML_EMIT_BLOCK_MAPPING_FIRST_KEY_STATE,
    /* Expect the key of a block mapping. */
    YAML_EMIT_BLOCK_MAPPING_KEY_STATE,
    /* Expect a value for a simple key of a block mapping. */
    YAML_EMIT_BLOCK_MAPPING_SIMPLE_VALUE_STATE,
    /* Expect a value of a block mapping. */
    YAML_EMIT_BLOCK_MAPPING_VALUE_STATE,
    /* Expect nothing. */
    YAML_EMIT_END_STATE
} yaml_emitter_state_t;

/*
 * The information of a node being emitted.
 */

struct typedef yaml_node_data_s {
    /* The node id. */
    int id;
    /* The collection iterator. */
    int index;
} yaml_node_data_t;

/*
 * The structure that holds data used by the file and string readers.
 */

typedef struct yaml_standard_writer_data_t {
    /* String output data. */
    yaml_ostring_t string;
    size_t *length;
    /* File output data. */
    FILE *file;
} yaml_standard_writer_data_t;

/*
 * The internal emitter structure.
 */

struct yaml_emitter_s {

    /*
     * Error stuff.
     */

    yaml_error_t error;

    /*
     * Writer stuff.
     */

    /* Write handler. */
    yaml_writer_t *writer;

    /* A pointer for passing to the white handler. */
    void *writer_data;

    /* Standard (string or file) output data. */
    yaml_standard_writer_data_t standard_writer_data;

    /* The working buffer. */
    yaml_iostring_t output;

    /* The raw buffer. */
    yaml_raw_iostring_t raw_output;

    /* The offset of the current position (in bytes). */
    size_t offset;

    /* The stream encoding. */
    yaml_encoding_t encoding;

    /*
     * Emitter stuff.
     */

    /* If the output is in the canonical style? */
    int is_canonical;
    /* The number of indentation spaces. */
    int best_indent;
    /* The preferred width of the output lines. */
    int best_width;
    /* Allow unescaped non-ASCII characters? */
    int is_unicode;
    /* The preferred line break. */
    yaml_break_t line_break;

    /* The stack of states. */
    struct {
        yaml_emitter_state_t *list;
        size_t length;
        size_t capacity;
    } states;

    /* The current emitter state. */
    yaml_emitter_state_t state;

    /* The event queue. */
    struct {
        yaml_event_t *list;
        size_t head;
        size_t tail;
        size_t capacity;
    } events;

    /* The stack of indentation levels. */
    struct {
        int *list;
        size_t length;
        size_t capacity;
    } indents;

    /* The list of tag directives. */
    struct {
        yaml_tag_directive_t *list;
        size_t length;
        size_t capacity;
    } tag_directives;

    /* The current indentation level. */
    int indent;

    /* The current flow level. */
    int flow_level;

    /* Is it the document root context? */
    int is_root_context;
    /* Is it a sequence context? */
    int is_sequence_context;
    /* Is it a mapping context? */
    int is_mapping_context;
    /* Is it a simple mapping key context? */
    int is_simple_key_context;

    /* The current line. */
    int line;
    /* The current column. */
    int column;
    /* If the last character was a whitespace? */
    int is_whitespace;
    /* If the last character was an indentation character (' ', '-', '?', ':')? */
    int is_indention;

    /* Anchor analysis. */
    struct {
        /* The anchor value. */
        const yaml_char_t *anchor;
        /* The anchor length. */
        size_t anchor_length;
        /* Is it an alias? */
        int is_alias;
    } anchor_data;

    /* Tag analysis. */
    struct {
        /* The tag handle. */
        const yaml_char_t *handle;
        /* The tag handle length. */
        size_t handle_length;
        /* The tag suffix. */
        const yaml_char_t *suffix;
        /* The tag suffix length. */
        size_t suffix_length;
    } tag_data;

    /* Scalar analysis. */
    struct {
        /* The scalar value. */
        const yaml_char_t *value;
        /* The scalar length. */
        size_t length;
        /* Does the scalar contain line breaks? */
        int is_multiline;
        /* Can the scalar be expessed in the flow plain style? */
        int is_flow_plain_allowed;
        /* Can the scalar be expressed in the block plain style? */
        int is_block_plain_allowed;
        /* Can the scalar be expressed in the single quoted style? */
        int is_single_quoted_allowed;
        /* Can the scalar be expressed in the literal or folded styles? */
        int is_block_allowed;
        /* The output style. */
        yaml_scalar_style_t style;
    } scalar_data;

    /*
     * Dumper stuff.
     */

    /* The resolve handler. */
    yaml_resolver_t *resolver;

    /* The application data to be passed to the resolver. */
    void *resolver_data;

    /* If the stream was already opened? */
    int is_opened;
    /* If the stream was already closed? */
    int is_closed;

    /* The information associated with the document nodes. */
    struct {
        /* The number of references. */
        size_t references;
        /* The anchor id. */
        int anchor;
        /* If the node has been emitted? */
        int is_serialized;
    } *anchors;

    /* The last assigned anchor id. */
    int last_anchor_id;

    /* The document being emitted. */
    yaml_document_t *document;

};

