APP_PLATFORM := android-15
NDK_TOOLCHAIN_VERSION := 4.9
APP_STL := gnustl_static
APP_ABI := armeabi-v7a #armeabi,armeabi-v7a,x86,mips,arm64-v8a,x86_64

APP_CPPFLAGS += -fexceptions -std=gnu++1y
APP_MODULES := nzbget
