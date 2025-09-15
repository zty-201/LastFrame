#include <jni.h>
#include <string>
#include <android/log.h>
// Ensure JNI macros are available
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
#include "GLRenderer.h"
// Define logging macros for this file
#ifndef LOG_TAG
#define LOG_TAG "NativeTemplate"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif
// Global renderer instance
static GLRenderer* g_renderer = nullptr;
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_jnicpp_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("stringFromJNI called");
    std::string hello = "OpenGL ES 3.0 Triangle Demo";
    return env->NewStringUTF(hello.c_str());
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_jnicpp_MainActivity_initGL(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("initGL called");
// Create renderer if not exists
    if (g_renderer == nullptr) {
        LOGI("Creating new GLRenderer instance");
        g_renderer = new GLRenderer();
        if (g_renderer == nullptr) {
            LOGE("Failed to create GLRenderer instance");
            return JNI_FALSE;
        }
    }
// Initialize OpenGL renderer (no longer needs surface parameter)
    bool success = g_renderer->initialize();
    if (success) {
        LOGI("OpenGL initialization successful");
    } else {
        LOGE("OpenGL initialization failed");
    }
    return success ? JNI_TRUE : JNI_FALSE;
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_renderFrame(
        JNIEnv* env,
        jobject /* this */) {
    if (g_renderer != nullptr) {
        g_renderer->render();
    } else {
        LOGE("Renderer is null in renderFrame");
    }
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_cleanupGL(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("cleanupGL called");
    if (g_renderer != nullptr) {
        g_renderer->cleanup();
        delete g_renderer;
        g_renderer = nullptr;
        LOGI("GLRenderer cleaned up and deleted");
    } else {
        LOGI("GLRenderer was already null");
    }
}