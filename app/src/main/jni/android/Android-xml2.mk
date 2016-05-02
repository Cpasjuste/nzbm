LOCAL_PATH := $(call my-dir)

common_SRC_FILES := ../libxml2/SAX.c ../libxml2/entities.c ../libxml2/encoding.c ../libxml2/error.c \
        ../libxml2/parserInternals.c ../libxml2/parser.c ../libxml2/tree.c ../libxml2/hash.c ../libxml2/list.c ../libxml2/xmlIO.c \
        ../libxml2/xmlmemory.c ../libxml2/uri.c ../libxml2/valid.c ../libxml2/xlink.c \
        ../libxml2/debugXML.c ../libxml2/xpath.c ../libxml2/xpointer.c ../libxml2/xinclude.c \
        ../libxml2/DOCBparser.c ../libxml2/catalog.c ../libxml2/globals.c ../libxml2/threads.c ../libxml2/c14n.c ../libxml2/xmlstring.c \
        ../libxml2/buf.c ../libxml2/xmlregexp.c ../libxml2/xmlschemas.c ../libxml2/xmlschemastypes.c ../libxml2/xmlunicode.c \
        ../libxml2/xmlreader.c ../libxml2/relaxng.c ../libxml2/dict.c ../libxml2/SAX2.c \
        ../libxml2/xmlwriter.c ../libxml2/legacy.c ../libxml2/chvalid.c ../libxml2/pattern.c ../libxml2/xmlsave.c ../libxml2/xmlmodule.c \
        ../libxml2/schematron.c \
        rand_r.c

common_C_INCLUDES += $(LOCAL_PATH)/../libxml2/include

common_CFLAGS += -DLIBXML_THREAD_ENABLED=1

common_CFLAGS += \
    -Wno-missing-field-initializers \
    -Wno-self-assign \
    -Wno-sign-compare \
    -Wno-unused-parameter \

include $(CLEAR_VARS)
LOCAL_SRC_FILES := $(common_SRC_FILES)
LOCAL_C_INCLUDES += $(common_C_INCLUDES)
LOCAL_CFLAGS += $(common_CFLAGS) -fvisibility=hidden
#LOCAL_SHARED_LIBRARIES += libicuuc
LOCAL_MODULE := libxml2
LOCAL_CLANG := true
LOCAL_ARM_MODE := arm
include $(BUILD_STATIC_LIBRARY)