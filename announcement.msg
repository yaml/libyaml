From: Ingy döt Net <ingy@ingy.net>
To: yaml-core@lists.sourceforge.net
Subject: [ANN] LibYAML-0.2.1: A new release

=========================
 Announcing LibYAML-0.2.1
=========================

A new MAJOR RELEASE of LibYAML is now available:
https://github.com/yaml/libyaml/tree/0.2.1

This is the first release of LibYAML under a new maintenance team. In August
2016, maintenance of LibYAML and PyYAML was turned over from the original
author, Kirill Simonov, to Ian Cordasco and Ingy döt Net.

The canonical source repo moved:

    from: https://bitbucket.org/xi/libyaml/
    to:   https://github.com/yaml/libyaml

The LibYAML project is now maintained by the YAML community. Planning happens
on the #yaml-dev and #libyaml IRC channels on irc.freenode.net.


Changes
=======

* https://github.com/yaml/libyaml/pull/7  -- Fixed most compiler warnings -Wall -Wextra
* https://github.com/yaml/libyaml/pull/10 -- Support static and dynamic libraries
* https://github.com/yaml/libyaml/pull/12 -- Use .gitignore instead of .hgignore
* https://github.com/yaml/libyaml/pull/13 -- Add support for `make test` and travis
* https://github.com/yaml/libyaml/pull/14 -- Dockerfile for testing
* https://github.com/yaml/libyaml/pull/15 -- Apply old fix for `\/` that is not in master.
* https://github.com/yaml/libyaml/pull/17 -- Update license to include all years until now.
* https://github.com/yaml/libyaml/pull/18 -- Port bug fix from Perl binding
* https://github.com/yaml/libyaml/pull/22 -- Fix misspell: preceed
* https://github.com/yaml/libyaml/pull/23 -- Removed trailing-whitespaces
* https://github.com/yaml/libyaml/pull/24 -- Fix typo
* https://github.com/yaml/libyaml/pull/25 -- added an examples directory with a few yaml examples
* https://github.com/yaml/libyaml/pull/26 -- Added missing Cflags path in pkg-config file
* https://github.com/yaml/libyaml/pull/31 -- add unit tests to cmake configuration
* https://github.com/yaml/libyaml/pull/32 -- Include an example of a custom tag from Python
* https://github.com/yaml/libyaml/pull/33 -- Include an example of a %YAML tag
* https://github.com/yaml/libyaml/pull/34 -- Added an example of using a global tag
* https://github.com/yaml/libyaml/pull/36 -- Fix -Wformat compilation errors in tests
* https://github.com/yaml/libyaml/pull/37 -- Update bug report URL in LibYAML
* https://github.com/yaml/libyaml/pull/38 -- Use AM_CPPFLAGS since autotools deprecated INCLUDE
* https://github.com/yaml/libyaml/pull/39 -- Update bug report URL in README
* https://github.com/yaml/libyaml/pull/41 -- Add travis and Makefile support for libyaml-test
* https://github.com/yaml/libyaml/pull/43 -- Add Dockerfile for Fedora 25
* https://github.com/yaml/libyaml/pull/44 -- WIP: Enable all warnings (-Wall) in libyaml and tests
* https://github.com/yaml/libyaml/pull/45 -- Fix typo
* https://github.com/yaml/libyaml/pull/47 -- Move travis script guts to separate file
* https://github.com/yaml/libyaml/pull/48 -- `yaml/libyaml-test` should become part of `yaml/libyaml`
* https://github.com/yaml/libyaml/pull/50 -- Add a GNUMakefile for immediate make targets
* https://github.com/yaml/libyaml/pull/53 -- Switch from test blacklist to whitelist
* https://github.com/yaml/libyaml/pull/55 -- Update defs for MingGW support on Windows
* https://github.com/yaml/libyaml/pull/58 -- Improve CMakeLists
* https://github.com/yaml/libyaml/pull/64 -- README: Update libyaml link
* https://github.com/yaml/libyaml/pull/69 -- Skip 5 tests in libyaml-emitter.list
* https://github.com/yaml/libyaml/pull/74 -- Forbid escaped singlequote in doublequotes
* https://github.com/yaml/libyaml/pull/77 -- Undefined PTRDIFF_MAX on HP-UX
* https://github.com/yaml/libyaml/pull/78 -- Fixed most compiler warnings -Wall -Wextra
* https://github.com/yaml/libyaml/pull/86 -- Fix problems in CI failures (travis and semaphore)
* https://github.com/yaml/libyaml/pull/87 -- appveyor.yml: add mingw-w64 builds
* https://github.com/yaml/libyaml/pull/88 -- add -no-undefined to src/Makefile.am
* https://github.com/yaml/libyaml/pull/89 -- Added alpine linux testing to dockerfiles
* https://github.com/yaml/libyaml/pull/93 -- remove need for PTRDIFF_MAX
* https://github.com/yaml/libyaml/pull/94 -- .gitignore: major cleanup


Resources
=========

LibYAML IRC Channel:  #libyaml on irc.freenode.net
LibYAML homepage:     https://github.com/yaml/libyaml
Source download:      https://github.com/yaml/libyaml/archive/dist-0.2.1.zip
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
* Ian Cordasco
* Tina Mueller

and many thanks to all who have contribributed!
See: https://github.com/yaml/libyaml/pulls


Copyright
=========

Copyright (c) 2017-2018 Ingy döt Net <ingy@ingy.net>
Copyright (c) 2006-2016 Kirill Simonov <xi@resolvent.net>

The LibYAML module was written by Kirill Simonov <xi@resolvent.net>.
It is currently maintained by the YAML community.

LibYAML is released under the MIT license.
See the file LICENSE for more details.
