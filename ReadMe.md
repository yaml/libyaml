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
It is used to test libyaml against the YAML Test Suite. The master branch has a
Makefile rule to run this using `make test-suite`.

That command will checkout this branch under the `tests/run-test-suite`
directory and then call this Makefile's `make test`.

See:

* https://github.com/yaml/yaml-test-suite

# Pinning

You can test older versions of master simply by checking out a version of
master and running `make test-suite`.

The HEAD commit of your master is checked against `conf/pin.tsv`. The first row
in the tsv file whose master-commit is found in your master history is used.
This will pin to the correct yaml-test-suite commit and the correct
run-test-suite-code branch commit.

The test code and whitelists are stored in the libyaml branch
`run-test-suite-code`.

NOTE: If no pinning is found, you will get a warning and the HEAD commit will
be used for the data and test code commits.

# Test Runner Usage

Print parse events for a YAML file (or stdin):
```
../run-parser-test-suite file.yaml
../run-parser-test-suite < file.yaml
cat file.yaml | ../run-parser-test-suite
```

Print the YAML for a libyaml-parser events file (or stdin):
```
../run-emitter-test-suite file.events
../run-emitter-test-suite < file.events
cat file.events | ../libyaml-run-test-suite
```

