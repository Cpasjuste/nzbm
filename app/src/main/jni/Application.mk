APP_PLATFORM := android-16
NDK_TOOLCHAIN_VERSION := 4.9
APP_STL := gnustl_static
APP_ABI := armeabi, armeabi-v7a, arm64-v8a, x86, x86_64, mips, mips64

APP_CPPFLAGS += -fexceptions -std=gnu++1y
APP_MODULES := nzbget libunrar
