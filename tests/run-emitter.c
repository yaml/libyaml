#include <yaml.h>

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

#define BUFFER_SIZE 65536
#define MAX_EVENTS  1024

int compare_events(const yaml_event_t *event, const yaml_event_t *model)
{
    yaml_char_t *event_anchor = NULL;
    yaml_char_t *model_anchor = NULL;
    yaml_char_t *event_tag = NULL;
    yaml_char_t *model_tag = NULL;
    yaml_char_t *event_value = NULL;
    yaml_char_t *model_value = NULL;
    size_t event_length = 0;
    size_t model_length = 0;
    int idx;

    if (event->type != model->type)
        return 0;

    switch (event->type)
    {
        case YAML_DOCUMENT_START_EVENT:
            if (!event->data.document_start.version_directive !=
                    !model->data.document_start.version_directive)
                return 0;
            if (event->data.document_start.version_directive) {
                yaml_version_directive_t *event_version_directive =
                    event->data.document_start.version_directive;
                yaml_version_directive_t *model_version_directive =
                    model->data.document_start.version_directive;
                if (event_version_directive->major !=
                        model_version_directive->major ||
                        event_version_directive->minor !=
                        model_version_directive->minor)
                    return 0;
            }
            if (event->data.document_start.tag_directives.length !=
                    model->data.document_start.tag_directives.length)
                return 0;
            for (idx = 0; idx < event->data.document_start.tag_directives.length; idx++) {
                yaml_tag_directive_t *event_tag_directive =
                    event->data.document_start.tag_directives.list + idx;
                yaml_tag_directive_t *model_tag_directive =
                    model->data.document_start.tag_directives.list + idx;
                if (strcmp(event_tag_directive->handle, model_tag_directive->handle))
                    return 0;
                if (strcmp(event_tag_directive->prefix, model_tag_directive->prefix))
                    return 0;
            }
            break;

        case YAML_ALIAS_EVENT:
            event_anchor = event->data.alias.anchor;
            model_anchor = model->data.alias.anchor;
            break;

        case YAML_SCALAR_EVENT:
            event_anchor = event->data.scalar.anchor;
            model_anchor = model->data.scalar.anchor;
            event_tag = event->data.scalar.tag;
            model_tag = model->data.scalar.tag;
            event_value = event->data.scalar.value;
            model_value = model->data.scalar.value;
            event_length = event->data.scalar.length;
            model_length = model->data.scalar.length;
            if (event->data.scalar.is_plain_implicit !=
                    model->data.scalar.is_plain_implicit)
                return 0;
            if (event->data.scalar.is_quoted_implicit !=
                    model->data.scalar.is_quoted_implicit)
                return 0;
            break;

        case YAML_SEQUENCE_START_EVENT:
            event_anchor = event->data.sequence_start.anchor;
            model_anchor = model->data.sequence_start.anchor;
            event_tag = event->data.sequence_start.tag;
            model_tag = model->data.sequence_start.tag;
            if (event->data.sequence_start.is_implicit !=
                    model->data.sequence_start.is_implicit)
                return 0;
            break;

        case YAML_MAPPING_START_EVENT:
            event_anchor = event->data.mapping_start.anchor;
            model_anchor = model->data.mapping_start.anchor;
            event_tag = event->data.mapping_start.tag;
            model_tag = model->data.mapping_start.tag;
            if (event->data.mapping_start.is_implicit !=
                    model->data.mapping_start.is_implicit)
                return 0;
            break;

        default:
            break;
    }

    if (!event_anchor != !model_anchor)
        return 0;
    if (event_anchor && strcmp(event_anchor, model_anchor))
        return 0;

    if (event_tag && !strcmp(event_tag, "!"))
        event_tag = NULL;
    if (model_tag && !strcmp(model_tag, "!"))
        model_tag = NULL;
    if (!event_tag != !model_tag)
        return 0;
    if (event_tag && strcmp(event_tag, model_tag))
        return 0;

    if (event_length != model_length)
        return 0;
    if (event_length && memcmp(event_value, model_value, event_length))
        return 0;

    return 1;
}

int print_output(char *name, unsigned char *buffer, size_t size, int count)
{
    FILE *file;
    char data[BUFFER_SIZE];
    size_t data_size = 1;
    size_t total_size = 0;
    if (count >= 0) {
        printf("FAILED (at the event #%d)\nSOURCE:\n", count+1);
    }
    file = fopen(name, "rb");
    assert(file);
    while (data_size > 0) {
        data_size = fread(data, 1, BUFFER_SIZE, file);
        assert(!ferror(file));
        if (!data_size) break;
        assert(fwrite(data, 1, data_size, stdout) == data_size);
        total_size += data_size;
        if (feof(file)) break;
    }
    fclose(file);
    printf("#### (length: %d)\n", total_size);
    printf("OUTPUT:\n%s#### (length: %d)\n", buffer, size);
    return 0;
}

int
main(int argc, char *argv[])
{
    int idx;
    int is_canonical = 0;
    int is_unicode = 0;

    idx = 1;
    while (idx < argc) {
        if (strcmp(argv[idx], "-c") == 0) {
            is_canonical = 1;
        }
        else if (strcmp(argv[idx], "-u") == 0) {
            is_unicode = 1;
        }
        else if (argv[idx][0] == '-') {
            printf("Unknown option: '%s'\n", argv[idx]);
            return 0;
        }
        if (argv[idx][0] == '-') {
            if (idx < argc-1) {
                memmove(argv+idx, argv+idx+1, (argc-idx-1)*sizeof(char *));
            }
            argc --;
        }
        else {
            idx ++;
        }
    }

    if (argc < 2) {
        printf("Usage: %s [-c] [-u] file1.yaml ...\n", argv[0]);
        return 0;
    }

    for (idx = 1; idx < argc; idx ++)
    {
        FILE *file;
        yaml_parser_t *parser;
        yaml_emitter_t *emitter;
        yaml_event_t event;
        unsigned char buffer[BUFFER_SIZE];
        size_t written = 0;
        yaml_event_t events[MAX_EVENTS];
        size_t event_number = 0;
        int count = 0;
        int failed = 0;

        memset(buffer, 0, BUFFER_SIZE);
        memset(events, 0, MAX_EVENTS*sizeof(yaml_event_t));

        printf("[%d] Parsing, emitting, and parsing again '%s': ", idx, argv[idx]);
        fflush(stdout);

        file = fopen(argv[idx], "rb");
        assert(file);

        parser = yaml_parser_new();
        assert(parser);

        yaml_parser_set_file_reader(parser, file);

        emitter = yaml_emitter_new();
        assert(emitter);

        if (is_canonical) {
            yaml_emitter_set_canonical(emitter, 1);
        }
        if (is_unicode) {
            yaml_emitter_set_unicode(emitter, 1);
        }
        yaml_emitter_set_string_writer(emitter, buffer, BUFFER_SIZE, &written);

        while (1)
        {
            if (!yaml_parser_parse_event(parser, &event))
                failed = 1;

            if (!event.type)
                break;

            assert(event_number < MAX_EVENTS);
            yaml_event_duplicate(&(events[event_number++]), &event);
            assert(yaml_emitter_emit_event(emitter, &event) || 
                    (yaml_emitter_flush(emitter) &&
                     print_output(argv[idx], buffer, written, count)));
            count ++;
        }

        yaml_parser_delete(parser);
        fclose(file);
        yaml_emitter_delete(emitter);

        if (!failed)
        {
            count = 0;
            parser = yaml_parser_new();
            yaml_parser_set_string_reader(parser, buffer, written);

            while (1)
            {
                assert(yaml_parser_parse_event(parser, &event) ||
                        print_output(argv[idx], buffer, written, count));
                if (!event.type)
                    break;
                assert(compare_events(events+count, &event) ||
                        print_output(argv[idx], buffer, written, count));
                yaml_event_destroy(&event);
                count ++;
            }
            yaml_parser_delete(parser);
        }

        while(event_number) {
            yaml_event_destroy(events+(--event_number));
        }

        printf("PASSED (length: %d)\n", written);
        print_output(argv[idx], buffer, written, -1);
    }

    return 0;
}
