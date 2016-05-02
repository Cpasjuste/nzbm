APP_PLATFORM := android-16
NDK_TOOLCHAIN_VERSION := 4.9
APP_STL := gnustl_shared
APP_ABI := armeabi-v7a #armeabi,armeabi-v7a,x86,mips,arm64-v8a,x86_64

APP_CPPFLAGS += -fexceptions -std=gnu++1y
APP_MODULES := nzbget libunrar
