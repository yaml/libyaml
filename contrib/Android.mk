#
# 2017, Christopher N. Hesse <christopher.hesse@delphi.com>
#

LOCAL_PATH := $(call my-dir)

YAML_SRC_PATH := ../src
YAML_INC_PATH := $(LOCAL_PATH)/../include

# from configure.ac
YAML_MAJOR := 0
YAML_MINOR := 1
YAML_PATCH := 7

YAML_LT_RELEASE := 0
YAML_LT_CURRENT := 2
YAML_LT_REVISION := 5
YAML_LT_AGE := 0


##########################################################################
# common yaml source files
YAML_COMMON_SRC := \
    $(YAML_SRC_PATH)/api.c \
    $(YAML_SRC_PATH)/dumper.c \
    $(YAML_SRC_PATH)/emitter.c \
    $(YAML_SRC_PATH)/loader.c \
    $(YAML_SRC_PATH)/parser.c \
    $(YAML_SRC_PATH)/reader.c \
    $(YAML_SRC_PATH)/scanner.c \
    $(YAML_SRC_PATH)/writer.c


##########################################################################
# libyaml (static)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(YAML_COMMON_SRC)

LOCAL_C_INCLUDES += $(YAML_INC_PATH)

LOCAL_CFLAGS += -DYAML_VERSION_MAJOR=$(YAML_MAJOR)
LOCAL_CFLAGS += -DYAML_VERSION_MINOR=$(YAML_MINOR)
LOCAL_CFLAGS += -DYAML_VERSION_PATCH=$(YAML_PATCH)
LOCAL_CFLAGS += -DYAML_VERSION_STRING=\"$(YAML_MAJOR).$(YAML_MINOR).$(YAML_PATCH)\"

#LOCAL_LDFLAGS += -release $(YAML_LT_RELEASE) -version-info $(YAML_LT_CURRENT):$(YAML_LT_REVISION):$(YAML_LT_AGE)

LOCAL_MODULE := libyaml

include $(BUILD_STATIC_LIBRARY)

##########################################################################
# libyaml (shared)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := $(YAML_COMMON_SRC)

LOCAL_C_INCLUDES += $(YAML_INC_PATH)

LOCAL_CFLAGS += -DYAML_VERSION_MAJOR=$(YAML_MAJOR)
LOCAL_CFLAGS += -DYAML_VERSION_MINOR=$(YAML_MINOR)
LOCAL_CFLAGS += -DYAML_VERSION_PATCH=$(YAML_PATCH)
LOCAL_CFLAGS += -DYAML_VERSION_STRING=\"$(YAML_MAJOR).$(YAML_MINOR).$(YAML_PATCH)\"

#LOCAL_LDFLAGS += -release $(YAML_LT_RELEASE) -version-info $(YAML_LT_CURRENT):$(YAML_LT_REVISION):$(YAML_LT_AGE)

LOCAL_MODULE := libyaml

include $(BUILD_SHARED_LIBRARY)
