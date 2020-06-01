LIBYAML_TEST_SUITE_URL         ?= https://github.com/yaml/yaml-test-suite
LIBYAML_TEST_SUITE_DATA_COMMIT ?= data-2020-02-11

.PHONY: clean test

test: data
	(cd data && git checkout $(LIBYAML_TEST_SUITE_DATA_COMMIT))
	prove -lv test

data:
	git clone $(LIBYAML_TEST_SUITE_URL) $@ --branch=$@

clean:
	rm -fr data
