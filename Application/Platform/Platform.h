/*
    A file include used for cross-platform compatibility for Windows and Android
*/
#ifndef PLATFORM_H
#define PLATFORM_H
// Platform detection
#ifdef _WIN32
#define PLATFORM_WINDOWS
#elif defined(__ANDROID__) && !defined(PLATFORM_ANDROID)
#define PLATFORM_ANDROID
#endif
// Platform-specific includes
#ifdef PLATFORM_WINDOWS
#include <iostream>
#include <cstring>
// Windows logging macros
#include <cstdio>
#define LOGI(...) printf("[INFO] " __VA_ARGS__); printf("\n")
#define LOGE(...) printf("[ERROR] " __VA_ARGS__); printf("\n")
#define LOGD(...) printf("[DEBUG] " __VA_ARGS__); printf("\n")
#elif defined(PLATFORM_ANDROID)
#include <android/log.h>
#include <cstring>
// Android logging macros (LOG_TAG should be defined in each source file)
#ifndef LOGI
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif
#endif
#endif // PLATFORM_H
