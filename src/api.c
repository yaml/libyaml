
#if HAVE_CONFIG_H
#include <config.h>
#endif

#include <yaml/yaml.h>

/*
 * Create a new parser.
 */

yaml_parser_t *
yaml_parser_new(void)
{
    yaml_parser_t *parser;

    parser = malloc(sizeof(yaml_parser_t));
    if (!parser) return NULL;

    memset(parser, 0, sizeof(yaml_parser_t));

    return parser;
}

/*
 * Destroy a parser object.
 */

void
yaml_parser_delete(yaml_parser_t *parser)
{
    free(parser);
}

