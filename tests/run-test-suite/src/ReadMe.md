libyaml-parser-emitter
======================

Parser and Emitter CLI tools for libyaml

# Synopsis

```
make build
make test
```

# Usage

Print parse events for a YAML file (or stdin):
```
./libyaml-parser file.yaml
./libyaml-parser < file.yaml
cat file.yaml | ./libyaml-parser
```

Print the YAML for a libyaml-parser events file (or stdin):
```
./libyaml-emitter file.events
./libyaml-emitter < file.events
cat file.events | ./libyaml-emitter
```

# Build

```
export LIBYAML_DIR=/path/to/libyaml   # Optional
make build
```
