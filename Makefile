.PHONY: test

PINNED_COMMITS := $(shell ./bin/pin)

$(eval MASTER_COMMIT = $(word 1, $(PINNED_COMMITS)))
$(eval LIST_COMMIT = $(word 2, $(PINNED_COMMITS)))
$(eval DATA_COMMIT = $(word 3, $(PINNED_COMMITS)))

default: help

help:
	@echo 'test  - Run the tests'
	@echo 'clean - Remove generated files'
	@echo 'help  - Show help'

test: data list
	prove -lv test

clean:
	rm -fr data list
	git worktree prune

data:
	git clone https://github.com/yaml/yaml-test-suite $@ --branch=$@
	(cd $@ && git reset --hard $(DATA_COMMIT))

list:
	-git branch --track run-test-suite-list origin/run-test-suite-list
	-git worktree prune
	git worktree add $@ run-test-suite-list
	(cd $@ && git reset --hard $(LIST_COMMIT))
