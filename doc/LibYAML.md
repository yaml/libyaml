# LibYAML

LibYAML is a YAML parser and emitter library.

## Download and Installation

The current release of LibYAML: *0.2.2 (2019-03-13)*.

Download the source package: <http://pyyaml.org/download/libyaml/yaml-0.2.2.tar.gz>.

To build and install LibYAML, run

    $ ./configure
    $ make
    # make install

You may check out the latest development code of LibYAML from the GitHub
repository <https://github.com/yaml/libyaml>:

    $ git clone https://github.com/yaml/libyaml

If you checked out the LibYAML source code from the GitHub repository, you can
build LibYAML with the commands:

    $ ./bootstrap
    $ ./configure
    $ make
    # make install

## Development and Bug Reports

You may check out the LibYAML source code from
[LibYAML Git repository](https://github.com/yaml/libyaml/).

If you find a bug in LibYAML, please
[file a bug report](https://github.com/yaml/libyaml/issues/).

You can discuss LibYAML with the developers on IRC: `#libyaml` irc.freenode.net

## Documentation

### Scope

LibYAML covers _presenting_ and _parsing_ [processes](http://yaml.org/spec/1.1/#id859458).
Thus LibYAML defines the following two processors:

* _Parser_, which takes an input stream of bytes and produces a sequence of parsing events.
* _Emitter_, which takes a sequence of events and produces a stream of bytes.

The processes of parsing and presenting are inverse to each other. Any sequence of events produced
by parsing a well-formed YAML document should be acceptable by the Emitter, which should produce
an equivalent document. Similarly, any document produced by emitting a sequence of events should be
acceptable for the Parser, which should produce an equivalent sequence of events.

The job of _resolving_ implicit tags, _composing_ and _serializing_ representation trees,
as well as _constructing_ and _representing_ native objects is left to applications and bindings.
Although some of these processes may be covered in the latter releases, they are not in the scope
of the initial release of LibYAML.

### Events

#### Event Types

The Parser produces while the Emitter accepts the following types of events:

* `STREAM-START`
* `STREAM-END`
* `DOCUMENT-START`
* `DOCUMENT-END`
* `ALIAS`
* `SCALAR`
* `SEQUENCE-START`
* `SEQUENCE-END`
* `MAPPING-START`
* `MAPPING-END`

A valid sequence of events should obey the grammar:

    stream ::= STREAM-START document* STREAM-END
    document ::= DOCUMENT-START node DOCUMENT-END
    node ::= ALIAS | SCALAR | sequence | mapping
    sequence ::= SEQUENCE-START node* SEQUENCE-END
    mapping ::= MAPPING-START (node node)* MAPPING-END

#### Essential Event Attributes

The following attributes affect the intepretation of a YAML document.

* `ALIAS`
    * `anchor` - the alias anchor; `[0-9a-zA-Z_-]+`; not `NULL`.
* `SCALAR`
    * `anchor` - the node anchor; `[0-9a-zA-Z_-]+`; may be `NULL`.
    * `tag` - the node tag; should either start with `!` (local tag) or be a valid URL (global tag);
      may be `NULL` or `!` in which case either `plain_implicit` or `quoted_implicit` should be `True`.
    * `plain_implicit` - `True` if the node tag may be omitted whenever the scalar value
      is presented in the plain style.
    * `quoted_implicit` - `True` if the node tag may be omitted whenever the scalar value
      is presented in any non-plain style.
    * `value` - the scalar value; a valid utf-8 sequence and may contain `NUL` characters; not `NULL`.
    * `length` - the length of the scalar value.
* `SEQUENCE-START`
    * `anchor` - the node anchor; `[0-9a-zA-Z_-]+`; may be `NULL`.
    * `tag` - the node tag; should either start with `!` (local tag) or be a valid URL (global tag);
      may be `NULL` or `!` in which case `implicit` should be `True`.
    * `implicit` - `True` if the node tag may be omitted.
* `MAPPING-START`
    * `anchor` - the node anchor; `[0-9a-zA-Z_-]+`; may be `NULL`.
    * `tag` - the node tag; should either start with `!` (local tag) or be a valid URL (global tag);
      may be `NULL` or `!` in which case `implicit` should be `True`.
    * `implicit` - `True` if the node tag may be omitted.

#### Stylistic Event Attributes

The following attributes don't affect the interpretation of a YAML document. While parsing a YAML
document, an application should not consider these attributes for resolving implicit tags and
constructing representation graphs or native objects. The Emitter may ignore these attributes
if they cannot be satisfied.

* `STREAM-START`
    * `encoding` - the document encoding; `utf-8|utf-16-le|utf-16-be`.
* `DOCUMENT-START`
    * `version_directive` - the version specified with the `%YAML` directive; the only valid value is `1.1`; may be `NULL`.
    * `tag_directives` - a set of tag handles and the corresponding tag prefixes specified with the `%TAG` directive;
      tag handles should match `!|!!|![0-9a-zA-Z_-]+!` while tag prefixes should be prefixes of valid local
      or global tags; may be empty.
    * `implicit` - `True` if the document start indicator `---` is not present.
* `DOCUMENT-END`
    * `implicit` - `True` if the document end indicator `...` is not present.
* `SCALAR`
    * `style` - the value style; `plain|single-quoted|double-quoted|literal|folded`.
* `SEQUENCE-START`
    * `style` - the sequence style; `block|flow`.
* `MAPPING-START`
    * `style` - the mapping style; `block|flow`.
* _any event_
    * `start_mark` - the position of the event beginning; attributes: `index` (in characters),
      `line` and `column` (starting from `0`).
    * `end_mark` - the position of the event end; attributes: `index` (in characters),
      `line` and `column` (starting from `0`).

### API

Note: the API may change drastically. You may also check the header file:
<https://github.com/yaml/libyaml/blob/master/include/yaml.h>

#### Parser API Synopsis

``` {.c}
#include <yaml.h>

yaml_parser_t parser;
yaml_event_t event;

int done = 0;

/* Create the Parser object. */
yaml_parser_initialize(&parser);

/* Set a string input. */
char *input = "...";
size_t length = strlen(input);

yaml_parser_set_input_string(&parser, input, length);

/* Set a file input. */
FILE *input = fopen("...", "rb");

yaml_parser_set_input_file(&parser, input);

/* Set a generic reader. */
void *ext = ...;
int read_handler(void *ext, char *buffer, int size, int *length) {
    /* ... */
    *buffer = ...;
    *length = ...;
    /* ... */
    return error ? 0 : 1;
}

yaml_parser_set_input(&parser, read_handler, ext);

/* Read the event sequence. */
while (!done) {

    /* Get the next event. */
    if (!yaml_parser_parse(&parser, &event))
        goto error;

    /*
      ...
      Process the event.
      ...
    */

    /* Are we finished? */
    done = (event.type == YAML_STREAM_END_EVENT);

    /* The application is responsible for destroying the event object. */
    yaml_event_delete(&event);

}

/* Destroy the Parser object. */
yaml_parser_delete(&parser);

return 1;

/* On error. */
error:

/* Destroy the Parser object. */
yaml_parser_delete(&parser);

return 0;
```

#### Emitter API Synopsis

``` {.c}
#include <yaml.h>

yaml_emitter_t emitter;
yaml_event_t event;

/* Create the Emitter object. */
yaml_emitter_initialize(&emitter);

/* Set a file output. */
FILE *output = fopen("...", "wb");

yaml_emitter_set_output_file(&emitter, output);

/* Set a generic writer. */
void *ext = ...;
int write_handler(void *ext, char *buffer, int size) {
    /*
       ...
       Write `size` bytes.
       ...
    */
    return error ? 0 : 1;
}

yaml_emitter_set_output(&emitter, write_handler, ext);

/* Create and emit the STREAM-START event. */
yaml_stream_start_event_initialize(&event, YAML_UTF8_ENCODING);
if (!yaml_emitter_emit(&emitter, &event))
    goto error;

/*
  ...
  Emit more events.
  ...
*/

/* Create and emit the STREAM-END event. */
yaml_stream_end_event_initialize(&event);
if (!yaml_emitter_emit(&emitter, &event))
    goto error;

/* Destroy the Emitter object. */
yaml_emitter_delete(&emitter);

return 1;

/* On error. */
error:

/* Destroy the Emitter object. */
yaml_emitter_delete(emitter);

return 0;
```

### Examples

You may check out
[tests](https://github.com/yaml/libyaml/tree/master/tests) and
[examples](https://github.com/yaml/libyaml/tree/master/examples) and
in the source distribution.


## Copyright

Copyright (c) 2017-2019 Ingy döt Net
Copyright (c) 2006-2016 Kirill Simonov

The LibYAML library was written by [Kirill Simonov](mailto:xi@resolvent.net).
It is now maintained by the YAML community.

LibYAML is released under the MIT license.

This project was developed for [Python Software Foundation](http://www.python.org/psf/)
as a part of [Google Summer of Code](http://code.google.com/soc/2006/psf/appinfo.html?csaid=75A3CBE3EC4B3DB2)
under the mentorship of [Clark Evans](http://clarkevans.com/).
