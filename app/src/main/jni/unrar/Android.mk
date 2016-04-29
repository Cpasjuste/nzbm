LOCAL_PATH:= $(call my-dir)

UNRAR_TOP := $(LOCAL_PATH)

include $(CLEAR_VARS)

LOCAL_SRC_FILES:= \
	rar.cpp strlist.cpp strfn.cpp pathfn.cpp smallfn.cpp global.cpp file.cpp filefn.cpp filcreat.cpp \
	archive.cpp arcread.cpp unicode.cpp system.cpp isnt.cpp crypt.cpp crc.cpp rawread.cpp encname.cpp \
	resource.cpp match.cpp timefn.cpp rdwrfn.cpp consio.cpp options.cpp errhnd.cpp rarvm.cpp \
	rijndael.cpp getbits.cpp sha1.cpp extinfo.cpp extract.cpp volume.cpp list.cpp find.cpp unpack.cpp cmddata.cpp \
	filestr.cpp recvol.cpp rs.cpp scantree.cpp hash.cpp secpassword.cpp rs16.cpp sha256.cpp blake2s.cpp ui.cpp \
	headers.cpp qopen.cpp

LOCAL_MODULE:= libunrar

LOCAL_C_INCLUDES := $(UNRAR_TOP) $(LOCAL_PATH)

LOCAL_CPPFLAGS := -D_ANDROID_EXE -D_FILE_OFFSET_BITS=64 -D_LARGEFILE_SOURCE -UALLOW_EXCEPTIONS

include $(BUILD_EXECUTABLE)

$(NDK_APP_LIBS_OUT)/%/libunrar.so: $(NDK_APP_LIBS_OUT)/%/unrar
		$(call host-mv, $<, $@)

all: $(foreach _abi,$(APP_ABI),$(NDK_APP_LIBS_OUT)/$(_abi)/libunrar.so)
