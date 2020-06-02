## Run `./bootstrap` to generate the "Makefile.in" files in this directory and
## the "$SUBDIRS" subdirectories.

SUBDIRS = include src . tests

EXTRA_DIST = Changes ReadMe.md License CMakeLists.txt doc/doxygen.cfg

export LIBYAML_ROOT := $(shell pwd)

LIBYAML_TEST := $(shell grep -E '^(http|git)' .libyaml-test | tail -n1)
LIBYAML_TEST_REPO ?= $(word 1, $(LIBYAML_TEST))
LIBYAML_TEST_COMMIT ?= $(word 2, $(LIBYAML_TEST))

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = yaml-0.1.pc

maintainer-clean-local:
	rm -f aclocal.m4 config.h.in configure config/*
	-find ${builddir} -name Makefile.in -exec rm -f '{}' ';'

distclean-local:
	rm -fr libyaml-test
	-git worktree prune

.PHONY: bootstrap libyaml-test
bootstrap: maintainer-clean
	./bootstrap
	./configure
	make

test-all: test test-suite

test: all
	make -C tests check-TESTS

test-suite: libyaml-test all
	make -C $< $@

docker-build:
	make -C pkg/docker build

docker-dist:
	make -C pkg/docker libyaml-dist

libyaml-test:
	-git clone $(LIBYAML_TEST_REPO) $@
	( cd $@ && git reset --hard $(LIBYAML_TEST_COMMIT) )
