
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml/yaml.h>

#include <assert.h>

/* Check for the UTF-16-BE BOM. */
#define IS_UTF16BE_BOM(pointer) ((pointer)[0] == 0xFE && (pointer)[1] == 0xFF)

/* Check for the UTF-16-LE BOM. */
#define IS_UTF16LE_BOM(pointer) ((pointer)[0] == 0xFF && (pointer)[1] == 0xFE)

/* Get a UTF-16-BE character. */
#define UTF16BE_CHAR(pointer)   ((pointer)[0] << 8 + (pointer)[1])

/* Get a UTF-16-LE character. */
#define UTF16LE_CHAR(pointer)   ((pointer)[0] + (pointer)[1] << 8)

/*
 * From http://www.ietf.org/rfc/rfc3629.txt:
 *
 *    Char. number range  |        UTF-8 octet sequence
 *      (hexadecimal)    |              (binary)
 *   --------------------+---------------------------------------------
 *   0000 0000-0000 007F | 0xxxxxxx
 *   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
 *   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
 *   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
 */

/* Get the length of a UTF-8 character (0 on error). */
#define UTF8_LENGTH(pointer)    \
    ((pointer)[0] < 0x80 ? 1 :  \
    (pointer)[0] < 0xC0 ? 0 :   \
    (pointer)[0] < 0xE0 ? 2 :   \
    (pointer)[0] < 0xF0 ? 3 :   \
    (pointer)[0] < 0xF8 ? 4 : 0)

/* Get the value of the first byte of a UTF-8 sequence (0xFF on error). */
#define UTF8_FIRST_CHUNK(pointer)   \
    ((pointer)[0] < 0x80 ? (pointer)[0] & 0x7F :    \
    (pointer)[0] < 0xC0 ? 0xFF :    \
    (pointer)[0] < 0xE0 ? (pointer)[0] & 0x1F : \
    (pointer)[0] < 0xF0 ? (pointer)[0] & 0x0F : \
    (pointer)[0] < 0xF8 ? (pointer)[0] & 0x07 : 0xFF)

/* Get the value of a non-first byte of a UTF-8 sequence (0xFF on error). */
#define UTF8_NEXT_CHUNK(pointer)    \
    ((pointer)[0] >= 0x80 && (pointer)[0] < 0xC0 ? (pointer)[0] & 0x3F : 0xFF)

/* Determine the length of a UTF-8 character. */

/*
 * Ensure that the buffer contains at least length characters.
 * Return 1 on success, 0 on failure.
 *
 * The length is supposed to be significantly less that the buffer size.
 */

int
yaml_parser_update_buffer(yaml_parser_t *parser, size_t length)
{
    /* If the EOF flag is set, do nothing. */

    if (parser->eof)
        return 1;

    /* Return if the buffer contains enough characters. */

    if (parser->unread >= length)
        return 1;

    /* Determine the input encoding if it is not known yet. */

    if (!parser->encoding) {
        if (!yaml_parser_determine_encoding(parser))
            return 0;
    }

    /* Move the unread characters to the beginning of the buffer. */

    if (parser->buffer < parser->pointer
            && parser->pointer < parser->buffer_end) {
        size_t size = parser->buffer_end - parser->pointer;
        memmove(parser->buffer, parser->pointer, size);
        parser->pointer = parser->buffer;
        parser->buffer_end -= size;
    }
    else if (parser->pointer == parser->buffer_end) {
        parser->pointer = parser->buffer;
        parser->buffer_end = parser->buffer;
    }

    /* Fill the buffer until it has enough characters. */

    while (parser->unread < length)
    {
        /* Fill the raw buffer. */

        if (!yaml_parser_update_raw_buffer(parser)) return 0;

        /* Decode the raw buffer. */

        while (parser->raw_unread)
        {
            unsigned int ch;
            int incomplete = 0;

            /* Decode the next character. */

            switch (parser->encoding)
            {
                case YAML_UTF8_ENCODING:

                    unsigned int utf8_length = UTF8_LENGTH(parser->raw_pointer);
                    unsigned int utf8_chunk;

                    /* Check if the raw buffer contains an incomplete character. */

                    if (utf8_length > parser->raw_unread) {
                        if (parser->eof) {
                            parser->error = YAML_READER_ERROR;
                            return 0;
                        }
                        incomplete = 1;
                    }

                    /* Get the character checking it for validity. */

                    utf8_chunk = UTF8_FIRST_CHUNK(parser->raw_pointer ++);
                    if (utf8_chunk == 0xFF) {
                        parser->error = YAML_READER_ERROR;
                        return 0;
                    }
                    ch = utf8_chunk;
                    parser->raw_unread --;
                    while (-- utf8_length) {
                        utf8_chunk = UTF8_NEXT_CHUNK(parser->raw_pointer ++);
                        if (utf8_chunk == 0xFF) {
                            parser->error = YAML_READER_ERROR;
                            return 0;
                        }
                        ch = ch << 6 + utf8_chunk;
                        parser->raw_unread --;
                    }

                    break;
                
                case YAML_UTF16LE_ENCODING:

                    /* Check if the raw buffer contains an incomplete character. */

                    if (parser->raw_unread < 2) {
                        if (parser->eof) {
                            parser->error = YAML_READER_ERROR;
                            return 0;
                        }
                        incomplete = 1;
                    }

                    /* Get the current character. */

                    ch = UTF16LE_CHAR(parser->raw_pointer);
                    parser->raw_pointer += 2;
                    parser->raw_unread -= 2;

                    break;

                case YAML_UTF16BE_ENCODING:

                    /* Check if the raw buffer contains an incomplete character. */

                    if (parser->raw_unread < 2) {
                        if (parser->eof) {
                            parser->error = YAML_READER_ERROR;
                            return 0;
                        }
                        incomplete = 1;
                    }

                    /* Get the current character. */

                    ch = UTF16BE_CHAR(parser->raw_pointer);
                    parser->raw_pointer += 2;
                    parser->raw_unread -= 2;

                    break;
            }

            /*
             * Check if the character is in the allowed range:
             *      #x9 | #xA | #xD | [#x20-#x7E]               (8 bit)
             *      | #x85 | [#xA0-#xD7FF] | [#xE000-#xFFFD]    (16 bit)
             *      | [#x10000-#x10FFFF]                        (32 bit)
             */

            if (! (ch == 0x09 || ch == 0x0A || ch == 0x0D
                        || (ch >= 0x20 && ch <= 0x7E)
                        || (ch == 0x85) || (ch >= 0xA0 && ch <= 0xD7FF)
                        || (ch >= 0xE000 && ch <= 0xFFFD)
                        || (ch >= 0x10000 && ch <= 0x10FFFF))) {
                parser->error = YAML_READER_ERROR;
                return 0;
            }

            /* Finally put the character into the buffer. */

            /* 0000 0000-0000 007F -> 0xxxxxxx */
            if (ch <= 0x7F) {
                *(parser->buffer_end++) = ch;
            }
            /* 0000 0080-0000 07FF -> 110xxxxx 10xxxxxx */
            else if (ch <= 0x7FF) {
                *(parser->buffer_end++) = 0xC0 + (ch >> 6) & 0x1F;
                *(parser->buffer_end++) = 0x80 + ch & 0x3F;
            }
            /* 0000 0800-0000 FFFF -> 1110xxxx 10xxxxxx 10xxxxxx */
            else if (ch <= 0xFFFF) {
                *(parser->buffer_end++) = 0x80 + ch & 0x3F;
                *(parser->buffer_end++) = 0xC0 + (ch >> 6) & 0x1F;
                
            }
            /* 0001 0000-0010 FFFF -> 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            else {
            }
        }
        
    }

}

/*
 * Determine the input stream encoding by checking the BOM symbol. If no BOM is
 * found, the UTF-8 encoding is assumed. Return 1 on success, 0 on failure.
 */

int
yaml_parser_determine_encoding(yaml_parser_t *parser)
{
    /* Ensure that we had enough bytes in the raw buffer. */

    while (!parser->eof && parser->raw_unread < 2) {
        if (!yaml_parser_update_raw_buffer(parser)) {
            return 0;
        }
    }

    /* Determine the encoding. */

    if (parser->raw_unread >= 2 && IS_UTF16BE_BOM(parser->raw_pointer)) {
        parser->encoding = YAML_UTF16BE_ENCODING;
    }
    else if (parser->raw_unread >= 2 && IS_UTF16LE_BOM(parser->raw_pointer)) {
        parser->encoding = YAML_UTF16LE_ENCODING;
    }
    else {
        parser->encoding = YAML_UTF8_ENCODING;
    }
}

/*
 * Update the raw buffer.
 */

int
yaml_parser_update_raw_buffer(yaml_parser_t *parser)
{
    size_t size_read = 0;

    /* Return if the raw buffer is full. */

    if (parser->raw_unread == YAML_RAW_BUFFER_SIZE) return 1;

    /* Return on EOF. */

    if (parser->eof) return 1;

    /* Move the remaining bytes in the raw buffer to the beginning. */

    if (parser->raw_unread && parser->raw_buffer < parser->raw_pointer) {
        memmove(parser->raw_buffer, parser->raw_pointer, parser->raw_unread);
    }
    parser->raw_pointer = parser->raw_buffer;

    /* Call the read handler to fill the buffer. */

    if (!parser->read_handler(parser->read_handler_data,
                parser->raw_buffer + parser->raw_unread,
                YAML_RAW_BUFFER_SIZE - parser->raw_unread,
                &size_read)) {
        parser->error = YAML_READER_ERROR;
        return 0;
    }
    parser->raw_unread += size_read;
    if (!size_read) {
        parser->eof = 1;
    }

    return 1;
}

