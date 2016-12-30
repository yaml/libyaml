.PHONY: test
GITHUB_ORG_URI := https://github.com/yaml
TEST_SUITE_URL := $(GITHUB_ORG_URI)/yaml-test-suite
LIBYAML_DIR ?= $(PWD)/libyaml-parser-emitter/libyaml

default: help

help:
	@echo 'test  - Run the tests'
	@echo 'clean - Remove generated files'
	@echo 'help  - Show help'

# Depends on parser and emitter but, building parser will also build emitter.
# Building twice makes things fail. Note: Some environments like on OS X, the
# shell resets the {DY,}LD_LIBRARY_PATH vars, so we work around it like so:
test: data src/libyaml-parser
	(export MY_LD_LIBRARY_PATH=$(LIBYAML_DIR)/src/.libs; prove -lv test)

clean:
	rm -fr data

data:
	git clone $(TEST_SUITE_URL) $@ --branch=$@

%/libyaml-parser %/libyaml-emitter: %
	(cd $<; make clean build)
	(cd $<; make clean build)

libyaml-parser-emitter:
	git clone $(GITHUB_ORG_URI)/$@ $@
