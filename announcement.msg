From: Tina Müller <post@tinita.de>
To: yaml-core@lists.sourceforge.net
Subject: [ANN] LibYAML-0.2.3: A new release

=========================
 Announcing LibYAML-0.2.3
=========================

A new release of LibYAML is now available:
https://github.com/yaml/libyaml/tree/0.2.3

The LibYAML project is now maintained by the YAML community. Planning happens
on the #yaml-dev and #libyaml IRC channels on irc.freenode.net.


Changes
=======

- https://github.com/yaml/libyaml/pull/66  -- Change dllexport controlling macro to use _WIN32
- https://github.com/yaml/libyaml/pull/127 -- Avoid recursion in the document loader.
- https://github.com/yaml/libyaml/pull/128 -- Squash a couple of warnings in example-deconstructor-alt
- https://github.com/yaml/libyaml/pull/130 -- Fixed typo.
- https://github.com/yaml/libyaml/pull/140 -- Use pointer to const for strings that aren't/shouldn't be modified
- https://github.com/yaml/libyaml/pull/144 -- Fix typo in comment
- https://github.com/yaml/libyaml/pull/151 -- Fix spelling for error message
- https://github.com/yaml/libyaml/pull/155 -- include/yaml.h: fix comments
- https://github.com/yaml/libyaml/pull/157 -- change cmake target name from libOFF.a to libyaml.a
- https://github.com/yaml/libyaml/pull/159 -- Add CHANGES file
- https://github.com/yaml/libyaml/pull/160 -- Always output document end before directive (YAML 1.2 compatibility)
- https://github.com/yaml/libyaml/pull/161 -- Make appveyor config be a hidden file
- https://github.com/yaml/libyaml/pull/162 -- Output document end marker after open ended scalars
- https://github.com/yaml/libyaml/pull/169 -- Fixed missing token in example
- https://github.com/yaml/libyaml/pull/172 -- Support %YAML 1.2 directives


Resources
=========

LibYAML IRC Channel:  #libyaml on irc.freenode.net
LibYAML homepage:     https://github.com/yaml/libyaml
Source download:      https://github.com/yaml/libyaml/archive/dist-0.2.3.zip
GitHub repository:    https://github.com/yaml/libyaml
Bug tracking:          https://github.com/yaml/libyaml/issues

YAML homepage: http://yaml.org/
YAML-core mailing list: http://lists.sourceforge.net/lists/listinfo/yaml-core


About LibYAML
=============

YAML is a data serialization format designed for human readability and
interaction with scripting languages.

LibYAML is a C library for parsing and emitting YAML. LibYAML is the underlying
parser/emitter code for YAML frameworks in many programming languages.


Maintainers
===========

The following people are responsible for maintaining LibYAML:

* Ingy döt Net
* Tina Mueller

and many thanks to all who have contribributed!
See: https://github.com/yaml/libyaml/pulls


Copyright
=========

Copyright (c) 2017-2020 Ingy döt Net <ingy@ingy.net>
Copyright (c) 2006-2016 Kirill Simonov <xi@resolvent.net>

The LibYAML module was written by Kirill Simonov.
It is currently maintained by the YAML community.

LibYAML is released under the MIT license.
See the file LICENSE for more details.
