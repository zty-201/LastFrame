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
// Windows logging macros
#include <stdio.h>
#define LOGE(...) do { printf("[ERROR] "); printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGI(...) do { printf("[INFO] ");  printf(__VA_ARGS__); printf("\n"); } while(0)
#define LOGD(...) do { printf("[DEBUG] "); printf(__VA_ARGS__); printf("\n"); } while(0)
#elif defined(PLATFORM_ANDROID)
#include <android/log.h>
#include <cstring>
// Android logging macros (LOG_TAG should be defined in each source file)
#ifndef LOGI
#define LOGE(...) do { __android_log_print(ANDROID_LOG_ERROR, "MyApp", __VA_ARGS__); } while(0)
#define LOGI(...) do { __android_log_print(ANDROID_LOG_INFO,  "MyApp", __VA_ARGS__); } while(0)
#define LOGD(...) do { __android_log_print(ANDROID_LOG_DEBUG, "MyApp", __VA_ARGS__); } while(0)
#endif
#endif
#endif // PLATFORM_H
