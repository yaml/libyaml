#include <yaml.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>

static const struct {
    const char *title;
    const char *input;
    const char *tag;
} cases[] = {
    {"fragment in tag directive prefix",
        "%TAG ! http://www.w3.org/2001/XMLSchema#\n---\n!foo bar\n",
        "http://www.w3.org/2001/XMLSchema#foo"},
    {"fragment in verbatim tag",
        "!<http://example.com/#foo> bar\n",
        "http://example.com/#foo"},
    {"fragment in local tag",
        "!foo#fragment bar\n",
        "!foo#fragment"},
    {"fragment in named handle suffix",
        "%TAG !e! http://example.com/\n---\n!e!foo#fragment bar\n",
        "http://example.com/foo#fragment"},
    {"escaped fragment in verbatim tag",
        "!<http://example.com/%23foo> bar\n",
        "http://example.com/#foo"},
    {"comments after directive and scalar",
        "%TAG !e! http://example.com/ # comment\n"
        "---\n!e!foo bar # comment\n",
        "http://example.com/foo"},
    {"comment after local tag",
        "!foo # comment\nbar\n",
        "!foo"}
};

int
main(void)
{
    size_t i;
    int failures = 0;

    for (i = 0; i < sizeof(cases)/sizeof(cases[0]); i++) {
        yaml_parser_t parser;
        yaml_document_t document;
        yaml_node_t *root;

        assert(yaml_parser_initialize(&parser));
        yaml_parser_set_input_string(&parser,
                (const unsigned char *)cases[i].input, strlen(cases[i].input));

        if (!yaml_parser_load(&parser, &document)) {
            fprintf(stderr, "%s: %s\n", cases[i].title,
                    parser.problem ? parser.problem : "no error detail");
            failures++;
        }
        else {
            root = yaml_document_get_root_node(&document);
            assert(root && root->type == YAML_SCALAR_NODE);
            assert(strcmp((const char *)root->tag, cases[i].tag) == 0);
            assert(root->data.scalar.length == 3);
            assert(memcmp(root->data.scalar.value, "bar", 3) == 0);
            yaml_document_delete(&document);
        }

        yaml_parser_delete(&parser);
    }

    return failures ? EXIT_FAILURE : EXIT_SUCCESS;
}
