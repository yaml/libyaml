SHELL := bash

PINNED_COMMITS := $(shell ./bin/pin)

$(eval MASTER_COMMIT = $(word 1, $(PINNED_COMMITS)))
$(eval CODE_COMMIT = $(word 2, $(PINNED_COMMITS)))
$(eval DATA_COMMIT = $(word 3, $(PINNED_COMMITS)))

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
	-git worktree prune

data:
	git clone https://github.com/yaml/yaml-test-suite $@ --branch=$@
	(cd $@ && git reset --hard $(DATA_COMMIT))

code:
	-git branch --track run-test-suite-code origin/run-test-suite-code
	-git worktree prune
	-git worktree add test run-test-suite-code
	(cd test && git reset --hard $(CODE_COMMIT))
