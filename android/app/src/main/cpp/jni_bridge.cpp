#ifdef __ANDROID__

#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <cstring>
#include "renderer.h"
#include "options.h"

#define LOG_TAG "MatrixJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static renderer* g_renderer = nullptr;

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeInit(
    JNIEnv* env, jobject obj, jobject surface, jint width, jint height) {
    
    LOGI("nativeInit called: %dx%d", width, height);
    
    // Get native window from surface
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window from surface");
        return;
    }
    
    // Create options
    options* opts = new options();
    opts->wallpaperMode = true;
    opts->width = width;
    opts->height = height;
    
    // Set app to matrix using safe string copy
    const char* appName = "matrix";
    size_t appNameLen = strlen(appName);
    if (appNameLen < 256) {
        memcpy(opts->app, appName, appNameLen + 1);
    } else {
        LOGE("App name too long");
        delete opts;
        return;
    }
    
    // Enable ghosting post-processing
    opts->postProcessingOptions = GHOSTING;
    opts->ghostingPreviousFrameOpacity = 0.997f;
    
    // Create renderer
    g_renderer = new renderer(opts);
    g_renderer->nativeWindow = window;
    
    try {
        g_renderer->initialize();
        LOGI("Renderer initialized successfully");
    } catch (const std::exception& e) {
        LOGE("Exception during renderer initialization: %s", e.what());
        delete g_renderer;
        g_renderer = nullptr;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeRender(JNIEnv* env, jobject obj) {
    if (g_renderer && !g_renderer->events->quit) {
        try {
            g_renderer->getEvents();
            g_renderer->frameBegin();
            g_renderer->loopApp();
            g_renderer->frameEnd();
            g_renderer->swapBuffers();
        } catch (const std::exception& e) {
            LOGE("Exception during render: %s", e.what());
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeDestroy(JNIEnv* env, jobject obj) {
    LOGI("nativeDestroy called");
    
    if (g_renderer) {
        try {
            g_renderer->destroy();
            delete g_renderer;
            g_renderer = nullptr;
            LOGI("Renderer destroyed successfully");
        } catch (const std::exception& e) {
            LOGE("Exception during renderer destruction: %s", e.what());
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeTouchEvent(
    JNIEnv* env, jobject obj, jfloat x, jfloat y, jboolean pressed) {
    
    if (g_renderer && g_renderer->events) {
        g_renderer->events->mouseX = static_cast<long>(x);
        g_renderer->events->mouseY = static_cast<long>(y);
        g_renderer->events->mouseLeft = pressed;
        if (pressed) {
            g_renderer->events->lastMouseMotion = g_renderer->clock->now();
        }
    }
}

#endif // __ANDROID__
