LIBYAML_HEAD_COMMIT ?= 0032321756cee86a67171de425267c1d0d406092
LIBYAML_MAIN_BRANCH ?= master
LIBYAML_REPO_URL ?= git@github.com:yaml/libyaml

TEST_SUITE_DATA_COMMIT ?= data-2020-02-11
TEST_SUITE_DATA_BRANCH ?= data
TEST_SUITE_REPO_URL ?= git@github.com:yaml/yaml-test-suite

.PHONY: test
test: libyaml/tests/run-parser-test-suite data
	prove -lv test/

libyaml/tests/run-parser-test-suite: libyaml
	( \
	    cd $< && \
	    ./bootstrap && \
	    ./configure && \
	    make \
	)

libyaml:
	git clone --branch=$(LIBYAML_MAIN_BRANCH) $(LIBYAML_REPO_URL) $@
	(cd $@ && git reset --hard $(LIBYAML_HEAD_COMMIT))

data:
	git clone --branch=$(TEST_SUITE_DATA_BRANCH) $(TEST_SUITE_REPO_URL) $@
	(cd $@ && git reset --hard $(TEST_SUITE_DATA_COMMIT))

clean:
	rm -fr libyaml data
