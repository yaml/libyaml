.PHONY: test
GITHUB_ORG_URI := https://github.com/yaml
TEST_SUITE_URL := $(GITHUB_ORG_URI)/yaml-test-suite

default: help

help:
	@echo 'test  - Run the tests'
	@echo 'clean - Remove generated files'
	@echo 'help  - Show help'

test: data
	prove -lv test

clean:
	rm -fr data

data:
	git clone $(TEST_SUITE_URL) $@ --branch=$@
	cd $@ && git reset --hard 44093f5e300695a6474999476e7e05c3451e1e49

