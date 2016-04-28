LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := xml2

LOCAL_SRC_FILES := SAX.c entities.c encoding.c error.c parserInternals.c  \
		parser.c tree.c hash.c list.c xmlIO.c xmlmemory.c uri.c  \
		valid.c xlink.c HTMLparser.c HTMLtree.c debugXML.c xpath.c  \
		xpointer.c xinclude.c nanohttp.c nanoftp.c DOCBparser.c \
		catalog.c globals.c threads.c c14n.c xmlstring.c \
		xmlregexp.c xmlschemas.c xmlschemastypes.c xmlunicode.c \
		xmlreader.c relaxng.c dict.c SAX2.c \
		xmlwriter.c legacy.c chvalid.c pattern.c xmlsave.c \
		xmlmodule.c schematron.c

LOCAL_CFLAGS += -DANDROID -DHAVE_CONFIG_H \
		-I$(LOCAL_PATH)/../libxml2/include

include $(BUILD_STATIC_LIBRARY)

