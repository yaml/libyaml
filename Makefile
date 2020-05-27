SHELL = bash

export ENV := $(shell pwd)/env

ifdef env
    export LIBYAML_TEST_SUITE_ENV := $(env)
endif

.ONESHELL:
.PHONY: test
test: data
	@set -ex
	[[ "$(debug)" ]] && export LIBYAML_TEST_SUITE_DEBUG=1
	export LIBYAML_TEST_SUITE_ENV=$$(./lookup env)
	[[ $$LIBYAML_TEST_SUITE_ENV ]] || exit 1
	prove -v test/

test-all:
	prove -v test/test-all.sh

data:
	@set -ex
	[[ "$(debug)" ]] && export LIBYAML_TEST_SUITE_DEBUG=1
	data=$$(./lookup data); repo=$${data%\ *}; commit=$${data#*\ }
	[[ $$data ]] || exit 1
	echo "repo=$$repo commit=$$commit"
	git clone $$repo $@
	(cd $@ && git reset --hard $$commit)

clean:
	rm -fr data
	rm -f env/tmp-*
