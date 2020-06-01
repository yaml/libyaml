run-test-suite
==============

Branch to support testing libyaml with yaml-test-suite

# Synopsis

From libyaml master branch:

```
./bootstrap
./configure
make test
make test-suite
```

# Overview

This code lives in the `libyaml` git repository on the `run-test-suite` branch.
It is used to test libyaml against the YAML Test Suite.
The master branch has a Makefile rule to run this using `make test-suite`.

That command will checkout this branch under the `tests/run-test-suite` directory and then call this Makefile's `make test`.

See:

* https://github.com/yaml/yaml-test-suite

# Pinning a Test Environment

By default, the test system will look for a file called `env/pin-<commit>` where `<commit>` is a git ref of the libyaml source code being tested (usually master).
When a commit is pushed to master, an entry should be added to the `env` directory for that commit.
The pin file contains information about how the tests should be run and will be sourced by the test runner.

Specifically, the pin file names the test suite repo, the commit to use, and which tests to use (blacklist/whitelist info).
The test suite repo is usually https://github.com/yaml/yaml-test-suite/, but could be a fork if you need to try something.

There are 2 commands: `whitelist` and `blacklist` which can be used to identify the appropriate tests to run.

Here is an example pin file:
```
# First set these 2 variables:
LIBYAML_TEST_SUITE_DATA_REPO=https://github.com/yaml/yaml-test-suite
LIBYAML_TEST_SUITE_DATA_COMMIT=data-2019-09-17

# Start by including all the tests:
whitelist all

# Next, blacklist or whitelist for `emitter`, `parser` and `parser-error`:

# Args can be a multiline string of IDs and descriptions:
blacklist parser "\
XXX1: A test
XXX2: Another test
"

# Use a predefined set of blacklists:
source "$ENV/blacklist-123"

# Args can also be a list of IDs:
whitelist emitter AAA1 AAA2 AAA3
whitelist parser-error XXX8 XXX9
```

Here's a pin file for just running a single parser test:
```
LIBYAML_TEST_SUITE_DATA_REPO=https://github.com/yaml/yaml-test-suite
LIBYAML_TEST_SUITE_DATA_COMMIT=data-2019-09-17
whitelist parser AAA1
```

## Overriding the Pin File

There are a few ways to specify which pinning to use.
They involve setting the `LIBYAML_TEST_SUITE_ENV` variable.

To use a specific pin file for testing, use an absolute path to some file:
```
make test-suite LIBYAML_TEST_SUITE_ENV=/tmp/my-pin-file
```

NOTE: You can use `env` instead of `LIBYAML_TEST_SUITE_ENV` with a `make test` command.

To use a specific pin file, name the file that lives in the `env` directory:
```
make test-suite env=pin-0032321756cee86a67171de425267c1d0d406092
```

## Specifying the Pin Info for a Pull Request

If you are submitting a PR, you can put the pin file content in your log message.
Add this exact line to your log message:
```
*** yaml-test-suite
```

Everything after that will be placed into a pin file.

## Pin Debugging

You can turn on debugging info for the pin `lookup` utility:
```
make test-suite debug=1
```
