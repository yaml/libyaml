
#define RAW_BUFFER_SIZE 16384
#define BUFFER_SIZE (RAW_BUFFER_SIZE*2) /* Should be enough for decoding 
                                           the whole raw buffer. */

/*
 * Ensure that the buffer contains at least length characters.
 * Return 1 on success, 0 on failure.
 */

int
yaml_parser_update_reader(yaml_parser_t *parser, size_t length)
{
    /* If the EOF flag is set, do nothing. */

    if (parser->eof)
        return 1;

    /* First, let us check that the buffers are allocated. */

    if (!parser->buffer) {
        parser->buffer = yaml_malloc(BUFFER_SIZE);
        if (!parser->buffer) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }
        parser->buffer_size = BUFFER_SIZE;
        parser->buffer_pointer = parser->buffer;
        parser->buffer_length = 0;
    }

    if (!parser->raw_buffer) {
        parser->raw_buffer = yaml_malloc(RAW_BUFFER_SIZE);
        if (!parser->raw_buffer) {
            parser->error = YAML_MEMORY_ERROR;
            return 0;
        }
        parser->raw_buffer_size = RAW_BUFFER_SIZE;
    }

    /* Next, determine the input encoding. */

    if (!parser->encoding) {
        if (!yaml_parser_determine_encoding(parser))
            return 0;
    }

    /* more... */

}



