From: Ingy döt Net <ingy@ingy.net>
To: yaml-core@lists.sourceforge.net
Subject: [ANN] LibYAML-0.2.2: A new release

=========================
 Announcing LibYAML-0.2.2
=========================

A new release of LibYAML is now available:
https://github.com/yaml/libyaml/tree/0.2.2

The LibYAML project is now maintained by the YAML community. Planning happens
on the #yaml-dev and #libyaml IRC channels on irc.freenode.net.


Changes
=======

* https://github.com/yaml/libyaml/pull/95 -- build: do not install config.h
* https://github.com/yaml/libyaml/pull/97 -- appveyor.yml: fix Release build
* https://github.com/yaml/libyaml/pull/103 -- Remove unused code in yaml_document_delete
* https://github.com/yaml/libyaml/pull/104 -- Allow colons in plain scalars inside flow collections
* https://github.com/yaml/libyaml/pull/109 -- Fix comparison in tests/run-emitter.c
* https://github.com/yaml/libyaml/pull/117 -- Fix typo error
* https://github.com/yaml/libyaml/pull/119 -- The closing single quote needs to be indented...
* https://github.com/yaml/libyaml/pull/121 -- fix token name typos in comments
* https://github.com/yaml/libyaml/pull/122 -- Revert removing of open_ended after top level plain scalar
* https://github.com/yaml/libyaml/pull/125 -- Cherry-picks from PR 27
* https://github.com/yaml/libyaml/pull/135 -- Windows/C89 compatibility
* https://github.com/yaml/libyaml/pull/136 -- allow override of Windows static lib name



Resources
=========

LibYAML IRC Channel:  #libyaml on irc.freenode.net
LibYAML homepage:     https://github.com/yaml/libyaml
Source download:      https://github.com/yaml/libyaml/archive/dist-0.2.2.zip
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

Copyright (c) 2017-2019 Ingy döt Net <ingy@ingy.net>
Copyright (c) 2006-2016 Kirill Simonov <xi@resolvent.net>

The LibYAML module was written by Kirill Simonov.
It is currently maintained by the YAML community.

LibYAML is released under the MIT license.
See the file LICENSE for more details.
