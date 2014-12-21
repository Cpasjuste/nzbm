LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_ARM_MODE := arm

LOCAL_MODULE := par2

LOCAL_SRC_FILES := commandline.cpp datablock.cpp galois.cpp par1fileformat.cpp par2creator.cpp par2repairersourcefile.cpp verificationhashtable.cpp \
		crc.cpp descriptionpacket.cpp libpar2.cpp par1repairer.cpp par2creatorsourcefile.cpp parheaders.cpp verificationpacket.cpp \
		creatorpacket.cpp diskfile.cpp mainpacket.cpp par1repairersourcefile.cpp par2fileformat.cpp recoverypacket.cpp \
		criticalpacket.cpp filechecksummer.cpp md5.cpp par2cmdline.cpp par2repairer.cpp reedsolomon.cpp

LOCAL_CFLAGS += -DANDROID -DHAVE_CONFIG_H \
		-I$(LOCAL_PATH)/../libsigc++-2.2.10

include $(BUILD_STATIC_LIBRARY)

