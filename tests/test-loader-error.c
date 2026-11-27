#include <yaml.h>

#include <stdio.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

typedef struct {
    yaml_error_type_t error;
    const char *problem;
    size_t problem_offset;
    int problem_value;
    yaml_mark_t problem_mark;
    const char *context;
    yaml_mark_t context_mark;
} parser_error_t;

static parser_error_t
save_parser_error(yaml_parser_t *parser)
{
    parser_error_t error;

    error.error = parser->error;
    error.problem = parser->problem;
    error.problem_offset = parser->problem_offset;
    error.problem_value = parser->problem_value;
    error.problem_mark = parser->problem_mark;
    error.context = parser->context;
    error.context_mark = parser->context_mark;

    return error;
}

static void
assert_parser_error(yaml_parser_t *parser, const parser_error_t *error)
{
    assert(parser->error == error->error);
    assert(parser->problem == error->problem);
    assert(parser->problem_offset == error->problem_offset);
    assert(parser->problem_value == error->problem_value);
    assert(parser->problem_mark.index == error->problem_mark.index);
    assert(parser->problem_mark.line == error->problem_mark.line);
    assert(parser->problem_mark.column == error->problem_mark.column);
    assert(parser->context == error->context);
    assert(parser->context_mark.index == error->context_mark.index);
    assert(parser->context_mark.line == error->context_mark.line);
    assert(parser->context_mark.column == error->context_mark.column);
}

static void
test_load_after_error(const unsigned char *input, size_t length,
        int stream_start_produced)
{
    yaml_parser_t parser;
    yaml_document_t document;
    parser_error_t error;
    int k;

    assert(yaml_parser_initialize(&parser));
    yaml_parser_set_input_string(&parser, input, length);

    assert(!yaml_parser_load(&parser, &document));
    assert(parser.error != YAML_NO_ERROR);
    assert(parser.stream_start_produced == stream_start_produced);
    error = save_parser_error(&parser);

    for (k = 0; k < 2; k++) {
        memset(&document, 0xA5, sizeof(document));
        assert(!yaml_parser_load(&parser, &document));
        assert_parser_error(&parser, &error);
        yaml_document_delete(&document);
    }

    yaml_parser_delete(&parser);
}

static void
test_multiple_documents(void)
{
    const unsigned char input[] = "---\nfirst\n---\nsecond\n";
    const char *values[] = { "first", "second" };
    yaml_parser_t parser;
    yaml_document_t document;
    yaml_node_t *root;
    int k;

    assert(yaml_parser_initialize(&parser));
    yaml_parser_set_input_string(&parser, input, sizeof(input)-1);

    for (k = 0; k < 2; k++) {
        assert(yaml_parser_load(&parser, &document));
        root = yaml_document_get_root_node(&document);
        assert(root);
        assert(root->type == YAML_SCALAR_NODE);
        assert(root->data.scalar.length == strlen(values[k]));
        assert(memcmp(root->data.scalar.value, values[k],
                    root->data.scalar.length) == 0);
        yaml_document_delete(&document);
    }

    assert(yaml_parser_load(&parser, &document));
    assert(!yaml_document_get_root_node(&document));
    yaml_document_delete(&document);

    yaml_parser_delete(&parser);
}

int
main(void)
{
    const unsigned char invalid_utf16[] = { 0xFF, 0xFE, 0x00, 0xD8 };
    const unsigned char invalid_yaml[] = "a: b: c\n";

    printf("Load after an error before STREAM-START ... ");
    test_load_after_error(invalid_utf16, sizeof(invalid_utf16), 0);
    printf("PASSED\n");

    printf("Load after an error after STREAM-START ... ");
    test_load_after_error(invalid_yaml, sizeof(invalid_yaml)-1, 1);
    printf("PASSED\n");

    printf("Load multiple valid documents ... ");
    test_multiple_documents();
    printf("PASSED\n");

    return 0;
}
