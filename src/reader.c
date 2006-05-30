
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml/yaml.h>

#include <assert.h>

/*
 * Set the reader error and return 0.
 */

int
yaml_parser_set_reader_error(yaml_parser_t *parser, const char *problem)
{
    parser->error = YAML_READER_ERROR;
    parser->problem = problem;
    parser->problem_offset = parser->offset;

    return 0;
}


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

        /* If the raw buffer is empty, it is EOF. */

        if (!parser->raw_unread) return 1;

        /* Decode the raw buffer. */

        while (parser->raw_unread)
        {
            unsigned int value, value2;
            int incomplete = 0;
            unsigned char utf8_octet;
            unsigned int utf8_length;
            int k, low, high;

            /* Decode the next character. */

            switch (parser->encoding)
            {
                case YAML_UTF8_ENCODING:

                    /*
                     * Decode a UTF-8 character.  Check RFC 3629
                     * (http://www.ietf.org/rfc/rfc3629.txt) for more details.
                     *
                     * The following table (taken from the RFC) is used for
                     * decoding.
                     *
                     *    Char. number range |        UTF-8 octet sequence
                     *      (hexadecimal)    |              (binary)
                     *   --------------------+------------------------------------
                     *   0000 0000-0000 007F | 0xxxxxxx
                     *   0000 0080-0000 07FF | 110xxxxx 10xxxxxx
                     *   0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
                     *   0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
                     *
                     * Additionally, the characters in the range 0xD800-0xDFFF
                     * are prohibited as they are reserved for use with UTF-16
                     * surrogate pairs.
                     */

                    /* Determine the length of the UTF-8 sequence. */

                    utf8_octet = parser->raw_pointer[0];
                    utf8_length = (
                            (utf8_octet & 0x80) == 0x00 ? 1 :
                            (utf8_octet & 0xE0) == 0xC0 ? 2 :
                            (utf8_octet & 0xF0) == 0xE0 ? 3 :
                            (utf8_octet & 0xF8) == 0xF0 ? 4 : 0);

                    /* Check if the leading octet is valid. */

                    if (!utf8_length)
                        return yaml_parser_set_reader_error(parser,
                                "Invalid leading UTF-8 octet");

                    /* Check if the raw buffer contains an incomplete character. */

                    if (utf8_length > parser->raw_unread) {
                        if (parser->eof) {
                            return yaml_parser_set_reader_error(parser,
                                    "Incomplete UTF-8 octet sequence");
                        }
                        incomplete = 1;
                        break;
                    }

                    /* Decode the leading octet. */

                    value = (
                        (utf8_octet & 0x80) == 0x00 ? utf8_octet & 0x7F :
                        (utf8_octet & 0xE0) == 0xC0 ? utf8_octet & 0x1F :
                        (utf8_octet & 0xF0) == 0xE0 ? utf8_octet & 0x0F :
                        (utf8_octet & 0xF8) == 0xF0 ? utf8_octet & 0x07 : 0);

                    /* Check and decode the trailing octets. */

                    for (k = 1; k < utf8_length; k ++)
                    {
                        utf8_octet = parser->raw_pointer[k];

                        /* Check if the octet is valid. */

                        if ((utf8_octet & 0xC0) != 0x80)
                            return yaml_parser_set_reader_error(parser,
                                    "Invalid trailing UTF-8 octet");

                        /* Decode the octet. */

                        value = (value << 6) + (utf8_octet & 0x3F);
                    }

                    /* Check the length of the sequence against the value. */

                    if (!((utf8_length == 1) ||
                            (utf8_length == 2 && value >= 0x80) ||
                            (utf8_length == 3 && value >= 0x800) ||
                            (utf8_length == 4 && value >= 0x10000)))
                        return yaml_parser_set_reader_error(parser,
                                "Invalid length of a UTF-8 sequence");

                    /* Check the range of the value. */

                    if ((value >= 0xD800 && value <= 0xDFFF) || value > 0x10FFFF)
                        return yaml_parser_set_reader_error(parser,
                                "Invalid Unicode character");

                    parser->raw_pointer += utf8_length;
                    parser->raw_unread -= utf8_length;
                    parser->offset += utf8_length;

                    break;
                
                case YAML_UTF16LE_ENCODING:
                case YAML_UTF16BE_ENCODING:

                    low = (parser->encoding == YAML_UTF16LE_ENCODING ? 0 : 1);
                    high = (parser->encoding == YAML_UTF16LE_ENCODING ? 1 : 0);

                    /*
                     * The UTF-16 encoding is not as simple as one might
                     * naively think.  Check RFC 2781
                     * (http://www.ietf.org/rfc/rfc2781.txt).
                     *
                     * Normally, two subsequent bytes describe a Unicode
                     * character.  However a special technique (called a
                     * surrogate pair) is used for specifying character
                     * values larger than 0xFFFF.
                     *
                     * A surrogate pair consists of two pseudo-characters:
                     *      high surrogate area (0xD800-0xDBFF)
                     *      low surrogate area (0xDC00-0xDFFF)
                     *
                     * The following formulas are used for decoding
                     * and encoding characters using surrogate pairs:
                     * 
                     *  U  = U' + 0x10000   (0x01 00 00 <= U <= 0x10 FF FF)
                     *  U' = yyyyyyyyyyxxxxxxxxxx   (0 <= U' <= 0x0F FF FF)
                     *  W1 = 110110yyyyyyyyyy
                     *  W2 = 110111xxxxxxxxxx
                     *
                     * where U is the character value, W1 is the high surrogate
                     * area, W2 is the low surrogate area.
                     */

                    /* Check for incomplete UTF-16 character. */

                    if (parser->raw_unread < 2) {
                        if (parser->eof) {
                            return yaml_parser_set_reader_error(parser,
                                    "Incomplete UTF-16 character");
                        }
                        incomplete = 1;
                        break;
                    }

                    /* Get the character. */

                    value = parser->raw_pointer[low]
                        + (parser->raw_pointer[high] << 8);

                    /* Check for unexpected low surrogate area. */

                    if ((value & 0xFC00) == 0xDC00)
                        return yaml_parser_set_reader_error(parser,
                                "Unexpected low surrogate area");

                    /* Check for a high surrogate area. */

                    if ((value & 0xFC00) == 0xD800) {

                        /* Check for incomplete surrogate pair. */

                        if (parser->raw_unread < 4) {
                            if (parser->eof) {
                                return yaml_parser_set_reader_error(parser,
                                        "Incomplete UTF-16 surrogate pair");
                            }
                            incomplete = 1;
                            break;
                        }

                        /* Get the next character. */

                        unsigned int value2 = parser->raw_pointer[low+2]
                            + (parser->raw_pointer[high+2] << 8);

                        /* Check for a low surrogate area. */

                        if ((value2 & 0xFC00) != 0xDC00)
                            return yaml_parser_set_reader_error(parser,
                                    "Expected low surrogate area");

                        /* Generate the value of the surrogate pair. */

                        value = 0x10000 + ((value & 0x3FF) << 10) + (value2 & 0x3FF);

                        parser->raw_pointer += 4;
                        parser->raw_unread -= 4;
                        parser->offset += 4;
                    }

                    else {
                        parser->raw_pointer += 2;
                        parser->raw_unread -= 2;
                        parser->offset += 4;
                    }

                    break;
            }

            /* Check if the raw buffer contains enough bytes to form a character. */

            if (incomplete) break;

            /*
             * Check if the character is in the allowed range:
             *      #x9 | #xA | #xD | [#x20-#x7E]               (8 bit)
             *      | #x85 | [#xA0-#xD7FF] | [#xE000-#xFFFD]    (16 bit)
             *      | [#x10000-#x10FFFF]                        (32 bit)
             */

            if (! (value == 0x09 || value == 0x0A || value == 0x0D
                        || (value >= 0x20 && value <= 0x7E)
                        || (value == 0x85) || (value >= 0xA0 && value <= 0xD7FF)
                        || (value >= 0xE000 && value <= 0xFFFD)
                        || (value >= 0x10000 && value <= 0x10FFFF)))
                return yaml_parser_set_reader_error(parser,
                        "Control characters are not allowed");

            /* Finally put the character into the buffer. */

            /* 0000 0000-0000 007F -> 0xxxxxxx */
            if (value <= 0x7F) {
                *(parser->buffer_end++) = value;
            }
            /* 0000 0080-0000 07FF -> 110xxxxx 10xxxxxx */
            else if (value <= 0x7FF) {
                *(parser->buffer_end++) = 0xC0 + (value >> 6);
                *(parser->buffer_end++) = 0x80 + value & 0x3F;
            }
            /* 0000 0800-0000 FFFF -> 1110xxxx 10xxxxxx 10xxxxxx */
            else if (value <= 0xFFFF) {
                *(parser->buffer_end++) = 0xE0 + (value >> 12);
                *(parser->buffer_end++) = 0x80 + (value >> 6) & 0x3F;
                *(parser->buffer_end++) = 0x80 + value & 0x3F;
            }
            /* 0001 0000-0010 FFFF -> 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
            else {
                *(parser->buffer_end++) = 0xF0 + (value >> 18);
                *(parser->buffer_end++) = 0x80 + (value >> 12) & 0x3F;
                *(parser->buffer_end++) = 0x80 + (value >> 6) & 0x3F;
                *(parser->buffer_end++) = 0x80 + value & 0x3F;
            }
        }
    }

    return 1;
}

/*
 * Determine the input stream encoding by checking the BOM symbol. If no BOM is
 * found, the UTF-8 encoding is assumed. Return 1 on success, 0 on failure.
 */

#define BOM_UTF8    "\xef\xbb\xbf"
#define BOM_UTF16LE "\xff\xfe"
#define BOM_UTF16BE "\xfe\xff"

int
yaml_parser_determine_encoding(yaml_parser_t *parser)
{
    /* Ensure that we had enough bytes in the raw buffer. */

    while (!parser->eof && parser->raw_unread < 3) {
        if (!yaml_parser_update_raw_buffer(parser)) {
            return 0;
        }
    }

    /* Determine the encoding. */

    if (parser->raw_unread >= 2
            && !memcmp(parser->raw_pointer, BOM_UTF16LE, 2)) {
        parser->encoding = YAML_UTF16LE_ENCODING;
        parser->raw_pointer += 2;
        parser->raw_unread -= 2;
    }
    else if (parser->raw_unread >= 2
            && !memcmp(parser->raw_pointer, BOM_UTF16BE, 2)) {
        parser->encoding = YAML_UTF16BE_ENCODING;
        parser->raw_pointer += 2;
        parser->raw_unread -= 2;
    }
    else if (parser->raw_unread >= 3
            && !memcmp(parser->raw_pointer, BOM_UTF8, 3)) {
        parser->encoding = YAML_UTF8_ENCODING;
        parser->raw_pointer += 3;
        parser->raw_unread -= 3;
    }
    else {
        parser->encoding = YAML_UTF8_ENCODING;
    }

    return 1;
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
        return yaml_parser_set_reader_error(parser, "Input error");
    }
    parser->raw_unread += size_read;
    if (!size_read) {
        parser->eof = 1;
    }

    return 1;
}

