.PHONY: test
GITHUB_ORG_URI := https://github.com/yaml
TEST_SUITE_URL := $(GITHUB_ORG_URI)/yaml-test-suite
LIBYAML_DIR ?= $(PWD)/libyaml-parser-emitter/libyaml

default: help

help:
	@echo 'test  - Run the tests'
	@echo 'clean - Remove generated files'
	@echo 'help  - Show help'

# Note: Some environments like on OS X, the
# shell resets the {DY,}LD_LIBRARY_PATH vars, so we work around it like so:
test: data
	(export MY_LD_LIBRARY_PATH=$(LIBYAML_DIR)/src/.libs; prove -lv test)

clean:
	rm -fr data

data:
	git clone $(TEST_SUITE_URL) $@ --branch=$@

