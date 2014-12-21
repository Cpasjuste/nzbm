LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := sigc

LOCAL_CPP_EXTENSION := .cc

LOCAL_SRC_FILES := ./sigc++/functors/slot.cc ./sigc++/functors/slot_base.cc ./sigc++/connection.cc \
			./sigc++/adaptors/lambda/lambda.cc ./sigc++/signal.cc ./sigc++/signal_base.cc ./sigc++/trackable.cc
			
LOCAL_CFLAGS += -DANDROID -DHAVE_CONFIG_H

include $(BUILD_STATIC_LIBRARY)

