APP_PLATFORM := android-9
NDK_TOOLCHAIN_VERSION := 4.8
APP_STL := gnustl_static
APP_ABI := armeabi,armeabi-v7a,x86,mips,arm64-v8a,x86_64

APP_CPPFLAGS += -fexceptions
APP_MODULES := libssl_static libcrypto_static xml2 nzbget
