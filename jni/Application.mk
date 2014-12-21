APP_STL := gnustl_static
APP_ABI := armeabi,armeabi-v7a,x86,mips,arm64-v8a,x86_64

APP_CPPFLAGS += -fexceptions
APP_MODULES := libssl_static libcrypto_static libsigc xml2 nzbget
#unrar libpar2 
