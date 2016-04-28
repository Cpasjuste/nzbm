#ifndef ANDROID_H
#define ANDROID_H

#include <stdio.h>
#include <jni.h>
#include <android/log.h>

#ifdef __cplusplus
extern "C" {
#endif

#define VERSION "17.0-testing"
#define PACKAGE_STRING "v17.0-r1686-15-g788d8d1"
#define PACKAGE_VERSION "17.0"

#ifndef LOG_TAG
#define LOG_TAG "nzbm"
#endif

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARNING,LOG_TAG,__VA_ARGS__)

#undef printf
#define printf(...) __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

#define F_TLOCK 0
int lockf(int fd, int cmd, off_t len);
int getdtablesize(void);

#ifdef __cplusplus
}
#endif

#define pthread_attr_setinheritsched
#define pthread_cancel( x ) pthread_kill( x, SIGUSR1 )
#define PTHREAD_INHERIT_SCHED 0

#endif
