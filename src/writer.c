
#include "yaml_private.h"

/*
 * Declarations.
 */

YAML_DECLARE(int)
yaml_emitter_flush(yaml_emitter_t *emitter);

/*
 * Flush the output buffer.
 */

YAML_DECLARE(int)
yaml_emitter_flush(yaml_emitter_t *emitter)
{
    int low, high;

    assert(emitter);    /* Non-NULL emitter object is expected. */
    assert(emitter->writer);    /* Write handler must be set. */
    assert(emitter->encoding);  /* Output encoding must be set. */

    /* Check if the buffer is empty. */

    if (!emitter->output.pointer) {
        return 1;
    }

    /* Switch the buffer into the input mode. */

    emitter->output.length = emitter->output.pointer;
    emitter->output.pointer = 0;

    /* If the output encoding is UTF-8, we don't need to recode the buffer. */

    if (emitter->encoding == YAML_UTF8_ENCODING)
    {
        if (emitter->writer(emitter->writer_data,
                    emitter->output.buffer, emitter->output.length)) {
            emitter->offset += emitter->output.length;
            emitter->output.length = 0;
            return 1;
        }
        else {
            return WRITER_ERROR_INIT(emitter,
                    "write handler error", emitter->offset);
        }
    }

    /* Recode the buffer into the raw buffer. */

    low = (emitter->encoding == YAML_UTF16LE_ENCODING ? 0 : 1);
    high = (emitter->encoding == YAML_UTF16LE_ENCODING ? 1 : 0);

    while (emitter->output.pointer < emitter->output.length)
    {
        unsigned char octet;
        unsigned int width;
        unsigned int value;
        size_t idx;

        /* 
         * See the "reader.c" code for more details on UTF-8 encoding.  Note
         * that we assume that the buffer contains a valid UTF-8 sequence.
         */

        /* Read the next UTF-8 character. */

        octet = OCTET(emitter->output);

        width = (octet & 0x80) == 0x00 ? 1 :
                (octet & 0xE0) == 0xC0 ? 2 :
                (octet & 0xF0) == 0xE0 ? 3 :
                (octet & 0xF8) == 0xF0 ? 4 : 0;

        value = (octet & 0x80) == 0x00 ? octet & 0x7F :
                (octet & 0xE0) == 0xC0 ? octet & 0x1F :
                (octet & 0xF0) == 0xE0 ? octet & 0x0F :
                (octet & 0xF8) == 0xF0 ? octet & 0x07 : 0;

        for (idx = 1; idx < width; idx ++) {
            octet = OCTET_AT(emitter->output, idx);
            value = (value << 6) + (octet & 0x3F);
        }

        emitter->output.pointer += width;

        /* Write the character. */

        if (value < 0x10000)
        {
            OCTET_AT(emitter->raw_output, high) = value >> 8;
            OCTET_AT(emitter->raw_output, low) = value & 0xFF;

            emitter->raw_output.pointer += 2;
        }
        else
        {
            /* Write the character using a surrogate pair (check "reader.c"). */

            value -= 0x10000;
            OCTET_AT(emitter->raw_output, high) = 0xD8 + (value >> 18);
            OCTET_AT(emitter->raw_output, low) = (value >> 10) & 0xFF;
            OCTET_AT(emitter->raw_output, high+2) = 0xDC + ((value >> 8) & 0xFF);
            OCTET_AT(emitter->raw_output, low+2) = value & 0xFF;

            emitter->raw_output.pointer += 4;
        }
    }

    /* Write the raw buffer. */

    if (emitter->writer(emitter->writer_data,
                emitter->raw_output.buffer, emitter->raw_output.pointer)) {
        emitter->output.pointer = 0;
        emitter->output.length = 0;
        emitter->offset += emitter->raw_output.pointer;
        emitter->raw_output.pointer = 0;
        return 1;
    }
    else {
        return WRITER_ERROR_INIT(emitter,
                "write handler error", emitter->offset);
    }
}

