## Run `./bootstrap` to generate the "Makefile.in" files in this directory and
## the "$SUBDIRS" subdirectories.

SUBDIRS = include src . tests

EXTRA_DIST = Changes ReadMe.md License CMakeLists.txt doc/doxygen.cfg

LIBYAML_TEST_SUITE_RUN_BRANCH ?= run-test-suite

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = yaml-0.1.pc

maintainer-clean-local:
	rm -f aclocal.m4 config.h.in configure config/*
	-find ${builddir} -name Makefile.in -exec rm -f '{}' ';'

distclean-local:
	rm -fr tests/run-test-suite packaging
	-git worktree prune

.PHONY: bootstrap
bootstrap: maintainer-clean
	./bootstrap
	./configure
	make

test: all
	make -C tests check-TESTS

test-suite: tests/run-test-suite all
	make -C $< test

test-all: test test-suite

tests/run-test-suite:
	-git branch --track $(LIBYAML_TEST_SUITE_RUN_BRANCH) origin/$(LIBYAML_TEST_SUITE_RUN_BRANCH)
	-git worktree prune
	git worktree add $@ $(LIBYAML_TEST_SUITE_RUN_BRANCH)

packaging:
	-git branch --track $@ origin/$@
	git worktree add --force $@ $@

docker-dist: packaging
	make -C $</docker libyaml-dist

docker-test-pyyaml: packaging
	make -C $</docker test-pyyaml
