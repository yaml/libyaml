LibYAML Test Runner
===================

Run run-parser-test-suite and run-emitter-test-suite against yaml-test-suite

# Synopsis

```
make test
# Run tests from yaml-test-suite
make test-suite
```

# Overview

See:

* https://github.com/yaml/yaml-test-suite

# Usage

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

