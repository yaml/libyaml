#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

/*
 * Test that the emitter writes characters outside the Basic Multilingual
 * Plane (#x10000-#x10FFFF, 4 bytes in UTF-8) unescaped if unescaped unicode
 * output is allowed, and that they are encoded correctly in UTF-16.
 */

#define BUFFER_SIZE 256

typedef struct {
    unsigned char data[BUFFER_SIZE];
    size_t length;
} output_t;

typedef struct {
    const char *title;
    yaml_encoding_t encoding;
    int unicode;
    yaml_scalar_style_t style;
    const char *value;
    const char *expected;
    size_t expected_length;
} test_case;

#define UTF8    YAML_UTF8_ENCODING
#define UTF16LE YAML_UTF16LE_ENCODING
#define UTF16BE YAML_UTF16BE_ENCODING
#define PLAIN   YAML_PLAIN_SCALAR_STYLE
#define DQUOTED YAML_DOUBLE_QUOTED_SCALAR_STYLE

test_case unicode_scalars[] = {
    /* {"title", encoding, unicode, style, "value", "expected", length}, */

    {"u-2600 is written unescaped", UTF8, 1, PLAIN,
        "\xe2\x98\x80", "\xe2\x98\x80\n", 4},
    {"u-FFFE is escaped", UTF8, 1, PLAIN,
        "\xef\xbf\xbe", "\"\\uFFFE\"\n", 9},
    {"u-10000 is written unescaped", UTF8, 1, PLAIN,
        "\xf0\x90\x80\x80", "\xf0\x90\x80\x80\n", 5},
    {"u-1F310 is written unescaped", UTF8, 1, PLAIN,
        "\xf0\x9f\x8c\x90", "\xf0\x9f\x8c\x90\n", 5},
    {"u-40000 is written unescaped", UTF8, 1, PLAIN,
        "\xf1\x80\x80\x80", "\xf1\x80\x80\x80\n", 5},
    {"u-10FFFF is written unescaped", UTF8, 1, PLAIN,
        "\xf4\x8f\xbf\xbf", "\xf4\x8f\xbf\xbf\n", 5},
    /* Not a Unicode character, but yaml_scalar_event_initialize accepts it;
     * it must not be written unescaped. */
    {"u-110000 is escaped", UTF8, 1, PLAIN,
        "\xf4\x90\x80\x80", "\"\\U00110000\"\n", 13},
    {"u-1F310 is written unescaped in a double quoted scalar", UTF8, 1, DQUOTED,
        "\xf0\x9f\x8c\x90", "\"\xf0\x9f\x8c\x90\"\n", 7},
    {"u-1F310 is escaped if unicode is not allowed", UTF8, 0, PLAIN,
        "\xf0\x9f\x8c\x90", "\"\\U0001F310\"\n", 13},

    {"u-1F310 in utf-16-le", UTF16LE, 1, PLAIN,
        "\xf0\x9f\x8c\x90", "\xff\xfe\x3c\xd8\x10\xdf\x0a\x00", 8},
    {"u-1F310 in utf-16-be", UTF16BE, 1, PLAIN,
        "\xf0\x9f\x8c\x90", "\xfe\xff\xd8\x3c\xdf\x10\x00\x0a", 8},
    {"u-10FFFF in utf-16-le", UTF16LE, 1, PLAIN,
        "\xf4\x8f\xbf\xbf", "\xff\xfe\xff\xdb\xff\xdf\x0a\x00", 8},

    {NULL, UTF8, 0, PLAIN, NULL, NULL, 0}
};

static int
write_handler(void *data, unsigned char *buffer, size_t size)
{
    output_t *output = (output_t *)data;

    if (output->length + size > BUFFER_SIZE)
        return 0;
    memcpy(output->data + output->length, buffer, size);
    output->length += size;
    return 1;
}

static int
emit_scalar(test_case *test, output_t *output)
{
    yaml_emitter_t emitter;
    yaml_event_t event;

    output->length = 0;

    assert(yaml_emitter_initialize(&emitter));
    yaml_emitter_set_output(&emitter, write_handler, output);
    yaml_emitter_set_unicode(&emitter, test->unicode);

    if (!yaml_stream_start_event_initialize(&event, test->encoding)
            || !yaml_emitter_emit(&emitter, &event))
        goto error;
    if (!yaml_document_start_event_initialize(&event, NULL, NULL, NULL, 1)
            || !yaml_emitter_emit(&emitter, &event))
        goto error;
    if (!yaml_scalar_event_initialize(&event, NULL, NULL,
                (yaml_char_t *)test->value, (int)strlen(test->value),
                1, 1, test->style)
            || !yaml_emitter_emit(&emitter, &event))
        goto error;
    if (!yaml_document_end_event_initialize(&event, 1)
            || !yaml_emitter_emit(&emitter, &event))
        goto error;
    if (!yaml_stream_end_event_initialize(&event)
            || !yaml_emitter_emit(&emitter, &event))
        goto error;

    yaml_emitter_delete(&emitter);
    return 1;

error:
    yaml_emitter_delete(&emitter);
    return 0;
}

static void
print_bytes(const unsigned char *data, size_t length)
{
    size_t k;

    for (k = 0; k < length; k ++) {
        printf("%02X ", data[k]);
    }
}

int check_unicode_scalars(void)
{
    output_t output;
    int failed = 0;
    int k;

    printf("checking unicode scalars...\n");
    for (k = 0; unicode_scalars[k].title; k ++) {
        test_case *test = unicode_scalars+k;

        printf("\t%s: ", test->title);
        if (!emit_scalar(test, &output)) {
            printf("- (emitter error)\n");
            failed ++;
        }
        else if (output.length != test->expected_length
                || memcmp(output.data, test->expected, output.length) != 0) {
            printf("- (got ");
            print_bytes(output.data, output.length);
            printf("while expected ");
            print_bytes((const unsigned char *)test->expected,
                    test->expected_length);
            printf(")\n");
            failed ++;
        }
        else {
            printf("+\n");
        }
    }
    printf("checking unicode scalars: %d fail(s)\n", failed);
    return failed;
}

int
main(void)
{
    return check_unicode_scalars();
}
