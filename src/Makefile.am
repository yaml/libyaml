AM_CPPFLAGS = -I$(top_srcdir)/include -Wall
lib_LTLIBRARIES = libyaml.la

if USE_VERSION_RC
VERSION_RC_FLAG = -Wl,version.o
VERSION_RC_OBJS = version.o
endif
libyaml_la_DEPENDENCIES = $(VERSION_RC_OBJS)
libyaml_la_SOURCES = yaml_private.h api.c reader.c scanner.c parser.c loader.c writer.c emitter.c dumper.c
libyaml_la_LDFLAGS = -no-undefined -release $(YAML_LT_RELEASE) -version-info $(YAML_LT_CURRENT):$(YAML_LT_REVISION):$(YAML_LT_AGE) $(VERSION_RC_FLAG)

%.o : %.rc
	$(WINDRES) -I$(top_srcdir)/include $< $@
