#ifndef ANDROID_H
#define ANDROID_H

#include <stdio.h>
#include <jni.h>
#include <android/log.h>
#include "Thread.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifndef LOG_TAG
#define LOG_TAG "nzbm"
#endif

#define LOGD(...)  __android_log_print(ANDROID_LOG_DEBUG,LOG_TAG,__VA_ARGS__)
#define LOGV(...)  __android_log_print(ANDROID_LOG_VERBOSE,LOG_TAG,__VA_ARGS__)
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)
#define  LOGE(...)  __android_log_print(ANDROID_LOG_ERROR,LOG_TAG,__VA_ARGS__)
#define  LOGW(...)  __android_log_print(ANDROID_LOG_WARNING,LOG_TAG,__VA_ARGS__)

//#define printf(...) LOGI(__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#define pthread_attr_setinheritsched
#define pthread_cancel( x ) pthread_kill( x, SIGUSR1 )
#define PTHREAD_INHERIT_SCHED 0

#endif

