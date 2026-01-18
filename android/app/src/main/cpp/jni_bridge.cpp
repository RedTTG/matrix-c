#ifdef __ANDROID__

#include <jni.h>
#include <android/native_window_jni.h>
#include <android/log.h>
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <cstring>
#include <mutex>
#include <thread>
#include <chrono>
#include "renderer.h"
#include "options.h"

#define LOG_TAG "MatrixJNI"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

static renderer* g_renderer = nullptr;
static std::mutex g_renderer_mutex;

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeInit(
    JNIEnv* env, jobject obj, jobject surface, jint width, jint height) {
    
    LOGI("nativeInit called: %dx%d", width, height);
    
    // Clean up any existing renderer first (lock only for the cleanup)
    {
        std::lock_guard<std::mutex> lock(g_renderer_mutex);
        if (g_renderer) {
            LOGI("Cleaning up existing renderer before creating new one");
            try {
                // Make sure the context is current before destroying
                if (g_renderer->eglDisplay != EGL_NO_DISPLAY &&
                    g_renderer->eglContext != EGL_NO_CONTEXT &&
                    g_renderer->eglSurface != EGL_NO_SURFACE) {
                    eglMakeCurrent(g_renderer->eglDisplay, g_renderer->eglSurface,
                                  g_renderer->eglSurface, g_renderer->eglContext);
                    glFinish(); // Wait for all GL operations to complete
                }

                g_renderer->destroy();
                delete g_renderer;
                g_renderer = nullptr;

                // Give Android's hwuiTask time to release resources
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } catch (const std::exception& e) {
                LOGE("Error cleaning up old renderer: %s", e.what());
            }
        }
    }

    // Get native window from surface
    ANativeWindow* window = ANativeWindow_fromSurface(env, surface);
    if (!window) {
        LOGE("Failed to get native window from surface");
        return;
    }
    
    LOGI("Native window obtained");

    // Create options
    options* opts = new options();
    opts->wallpaperMode = true;
    opts->width = width;
    opts->height = height;
    
    // Set app to matrix
    const char* appName = "matrix";
    size_t appNameLen = strlen(appName);
    if (appNameLen < 256) {
        memcpy(opts->app, appName, appNameLen + 1);
    } else {
        LOGE("App name too long");
        delete opts;
        ANativeWindow_release(window);
        return;
    }
    
    LOGI("Options configured for app: %s", opts->app);

    // Note: Post-processing options (ghosting, etc.) are configured by the app itself
    // Don't override them here - let the matrix app set them up

    // Create renderer (without holding the mutex during GL operations)
    LOGI("Creating renderer...");
    renderer* new_renderer = new renderer(opts);
    new_renderer->nativeWindow = window;

    try {
        LOGI("Starting makeContext...");
        new_renderer->makeContext();
        LOGI("makeContext completed");

        LOGI("Starting makeFrameBuffers...");
        new_renderer->makeFrameBuffers();
        LOGI("makeFrameBuffers completed");

        LOGI("Initializing clock...");
        new_renderer->clock->initialize();
        LOGI("Clock initialized");

        LOGI("Loading app...");
        new_renderer->loadApp();
        LOGI("App loaded");

        LOGI("Masking post-processing options...");
        new_renderer->opts->maskPostProcessingOptionsWithUserAllowed();
        LOGI("Post-processing options masked");

        LOGI("Initializing post-processing...");
        new_renderer->initializePP();
        LOGI("Post-processing initialized");

        LOGI("Renderer fully initialized successfully");

        // Only now assign to global pointer (lock for assignment only)
        {
            std::lock_guard<std::mutex> lock(g_renderer_mutex);
            g_renderer = new_renderer;
        }
    } catch (const std::exception& e) {
        LOGE("FATAL: Exception during initialization: %s", e.what());
        try {
            new_renderer->destroy();
        } catch (...) {}
        delete new_renderer;
        return;
    } catch (...) {
        LOGE("FATAL: Unknown exception during initialization");
        try {
            new_renderer->destroy();
        } catch (...) {}
        delete new_renderer;
        return;
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeRender(JNIEnv* env, jobject obj) {
    // Get renderer pointer with lock, then render without lock
    renderer* rnd = nullptr;
    {
        std::lock_guard<std::mutex> lock(g_renderer_mutex);
        rnd = g_renderer;
    }

    if (!rnd) {
        return;
    }

    if (rnd->events->quit) {
        return;
    }

    static int frameCount = 0;
    static bool loggedFirstFrames = false;

    try {
        rnd->getEvents();

        if (!loggedFirstFrames && frameCount < 3) {
            LOGI("Frame %d: Before frameBegin", frameCount);
        }
        rnd->frameBegin();

        if (!loggedFirstFrames && frameCount < 3) {
            LOGI("Frame %d: Before loopApp", frameCount);
        }
        rnd->loopApp();

        if (!loggedFirstFrames && frameCount < 3) {
            LOGI("Frame %d: Before frameEnd", frameCount);
        }
        rnd->frameEnd();

        if (!loggedFirstFrames && frameCount < 3) {
            LOGI("Frame %d: Before swapBuffers", frameCount);
        }
        rnd->swapBuffers();

        // Log first few frames for debugging
        if (!loggedFirstFrames) {
            frameCount++;
            if (frameCount <= 3) {
                LOGI("Rendered frame %d successfully", frameCount);
            } else if (frameCount == 4) {
                LOGI("Rendering working, suppressing further logs");
                loggedFirstFrames = true;
            }
        }
    } catch (const std::exception& e) {
        LOGE("Exception during render: %s", e.what());
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeDestroy(JNIEnv* env, jobject obj) {
    std::lock_guard<std::mutex> lock(g_renderer_mutex);

    LOGI("nativeDestroy called");
    
    if (g_renderer) {
        try {
            g_renderer->destroy();
            delete g_renderer;
            g_renderer = nullptr;
            LOGI("Renderer destroyed successfully");
        } catch (const std::exception& e) {
            LOGE("Exception during renderer destruction: %s", e.what());
            g_renderer = nullptr;
        }
    }
}

extern "C" JNIEXPORT void JNICALL
Java_com_redttg_matrix_MatrixWallpaperService_nativeTouchEvent(
    JNIEnv* env, jobject obj, jfloat x, jfloat y, jboolean pressed) {
    
    std::lock_guard<std::mutex> lock(g_renderer_mutex);

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
