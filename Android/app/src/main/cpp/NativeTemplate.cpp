#include <jni.h>
#include <string>
#include <android/log.h>
#include <android/asset_manager.h>
#include <android/asset_manager_jni.h>

#include "InputManager.hpp"

// Ensure JNI macros are available
#ifndef JNIEXPORT
#define JNIEXPORT
#endif
#ifndef JNICALL
#define JNICALL
#endif
#include "main.hpp"
// Define logging macros for this file
#ifndef LOG_TAG
#define LOG_TAG "NativeTemplate"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__)
#endif
// Global renderer instance
extern "C" JNIEXPORT jstring JNICALL
Java_com_example_jnicpp_MainActivity_stringFromJNI(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("stringFromJNI called");

    std::string hello = "LastFrame";
    return env->NewStringUTF(hello.c_str());
}
extern "C" JNIEXPORT jboolean JNICALL
Java_com_example_jnicpp_MainActivity_initGL(
        JNIEnv* env,
        jobject /* this */,
        jobject assetManager) {

    LOGI("initGL called with AssetManager");
    //Ensure garbage collector does not free the reference up.
    env->NewGlobalRef(assetManager);
    // Get AssetManager from Java object
    AAssetManager* nativeAssetManager = nullptr;
    if (assetManager != nullptr) {
        nativeAssetManager = AAssetManager_fromJava(env, assetManager);
        if (nativeAssetManager == nullptr) {
            LOGE("Failed to get native AssetManager");
            return JNI_FALSE;
        }
        LOGI("AssetManager obtained successfully");
    } else {
        LOGE("AssetManager is null");
        return JNI_FALSE;
    }
    //Pass the asset manager to c++ code.
    Game_Initialize((void*)nativeAssetManager);
    return JNI_TRUE;
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_renderFrame(
        JNIEnv* env,
        jobject /* this */) {
//    if (g_renderer != nullptr) {
//        g_renderer->render();
//    } else {
//        LOGE("Renderer is null in renderFrame");
//    }
//TODO: Run GameUpdate once, checking if the window is still valid.
    Game_Update();
}
extern "C" JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_cleanupGL(
        JNIEnv* env,
        jobject /* this */) {
    LOGI("cleanupGL called");
//    if (g_renderer != nullptr) {
//        g_renderer->cleanup();
//        delete g_renderer;
//        g_renderer = nullptr;
//        LOGI("GLRenderer cleaned up and deleted");
//    } else {
//        LOGI("GLRenderer was already null");
//    }
    Game_Destroy();
}

//JNI function to handle touch motion events (from Java's onTouchEvent)
extern "C" JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_handleNativeTouchEvent(
        JNIEnv* env,
        jobject /* this */,
        jint action,
        jfloat x,
        jfloat y) {

    //This calls static C++ function in InputManager
    EngineFramework::InputManager::UpdateTouch(action, x, y);
}

//JNI function to handle key events (from Java's onKeyDown/Up)
extern "C" JNIEXPORT void JNICALL
Java_com_example_jnicpp_MainActivity_handleNativeKeyEvent(
        JNIEnv* env,
        jobject /* this */,
        jint keyCode,
        jboolean isDown) {

    //This calls static C++ function in InputManager
    EngineFramework::InputManager::UpdateKey(keyCode, isDown);
}