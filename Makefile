SHELL := bash

PINNED_COMMITS := $(shell LIBYAML_DEBUG_PIN=$(LIBYAML_DEBUG_PIN) ./bin/pin)

ifeq ($(PINNED_COMMITS),)
    $(error ./bin/pin failed)
endif

LIBYAML_TEST_SUITE_CODE_COMMIT ?= $(word 2, $(PINNED_COMMITS))
LIBYAML_TEST_SUITE_DATA_COMMIT ?= $(word 3, $(PINNED_COMMITS))

default: help

help:
	@echo 'test  - Run the tests'
	@echo 'clean - Remove generated files'
	@echo 'help  - Show help'

.PHONY: test
test: data code
	prove -lv test

clean:
	rm -fr data test
	git worktree prune

data:
	git clone https://github.com/yaml/yaml-test-suite $@ --branch=$@
	(cd $@ && git reset --hard $(LIBYAML_TEST_SUITE_DATA_COMMIT))

code:
	-git branch --track run-test-suite-code origin/run-test-suite-code
	git worktree prune
	[[ -d test ]] || \
	    git worktree add test run-test-suite-code
	(cd test && git reset --hard $(LIBYAML_TEST_SUITE_CODE_COMMIT))
