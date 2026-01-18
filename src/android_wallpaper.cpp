#ifdef __ANDROID__

#include "android_wallpaper.h"
#include "renderer.h"
#include "gl_errors.h"
#include <iostream>
#include <android/log.h>

#define LOG_TAG "MatrixWallpaper"
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, LOG_TAG, __VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)

void initializeGladES() {
    // For Android with OpenGL ES 3.0, GLAD needs to be initialized with eglGetProcAddress
    if (!gladLoadGLES2Loader((GLADloadproc)eglGetProcAddress)) {
        LOGE("Failed to initialize GLAD for OpenGL ES");
        exit(1);
    }
    LOGI("GLAD initialized successfully for OpenGL ES 3.0");
}

void setupEGLForWallpaper(renderer *rnd, ANativeWindow* window) {
    LOGI("Setting up EGL for wallpaper mode");
    
    // Get EGL display
    rnd->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (rnd->eglDisplay == EGL_NO_DISPLAY) {
        LOGE("Failed to get EGL display");
        exit(1);
    }
    
    // Initialize EGL
    EGLint major, minor;
    if (!eglInitialize(rnd->eglDisplay, &major, &minor)) {
        LOGE("Failed to initialize EGL");
        exit(1);
    }
    LOGI("EGL initialized: version %d.%d", major, minor);
    
    // Configure EGL attributes
    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_SAMPLES, rnd->antialiasSamples,
        EGL_NONE
    };
    
    // Choose EGL config
    EGLint numConfigs;
    if (!eglChooseConfig(rnd->eglDisplay, attribs, &rnd->eglConfig, 1, &numConfigs) || numConfigs == 0) {
        LOGE("Failed to choose EGL config");
        exit(1);
    }
    LOGI("EGL config chosen successfully");
    
    // Create EGL context
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    rnd->eglContext = eglCreateContext(rnd->eglDisplay, rnd->eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (rnd->eglContext == EGL_NO_CONTEXT) {
        LOGE("Failed to create EGL context");
        exit(1);
    }
    LOGI("EGL context created successfully");
    
    // Create EGL surface
    rnd->nativeWindow = window;
    rnd->eglSurface = eglCreateWindowSurface(rnd->eglDisplay, rnd->eglConfig, window, nullptr);
    if (rnd->eglSurface == EGL_NO_SURFACE) {
        LOGE("Failed to create EGL surface");
        exit(1);
    }
    LOGI("EGL surface created successfully");
    
    // Make context current
    if (!eglMakeCurrent(rnd->eglDisplay, rnd->eglSurface, rnd->eglSurface, rnd->eglContext)) {
        LOGE("Failed to make EGL context current");
        exit(1);
    }
    
    // Get window dimensions
    rnd->opts->width = ANativeWindow_getWidth(window);
    rnd->opts->height = ANativeWindow_getHeight(window);
    LOGI("Window dimensions: %ldx%ld", rnd->opts->width, rnd->opts->height);
}

void android_SwapBuffers(renderer *rnd) {
    if (rnd->eglDisplay != EGL_NO_DISPLAY && rnd->eglSurface != EGL_NO_SURFACE) {
        eglSwapBuffers(rnd->eglDisplay, rnd->eglSurface);
    }
}

void destroyEGL(renderer *rnd) {
    LOGI("Destroying EGL resources");
    
    if (rnd->eglDisplay != EGL_NO_DISPLAY) {
        eglMakeCurrent(rnd->eglDisplay, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        
        if (rnd->eglSurface != EGL_NO_SURFACE) {
            eglDestroySurface(rnd->eglDisplay, rnd->eglSurface);
            rnd->eglSurface = EGL_NO_SURFACE;
        }
        
        if (rnd->eglContext != EGL_NO_CONTEXT) {
            eglDestroyContext(rnd->eglDisplay, rnd->eglContext);
            rnd->eglContext = EGL_NO_CONTEXT;
        }
        
        eglTerminate(rnd->eglDisplay);
        rnd->eglDisplay = EGL_NO_DISPLAY;
    }
    
    if (rnd->nativeWindow != nullptr) {
        ANativeWindow_release(rnd->nativeWindow);
        rnd->nativeWindow = nullptr;
    }
}

#endif // __ANDROID__
