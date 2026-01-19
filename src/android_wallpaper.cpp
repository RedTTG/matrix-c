#ifdef __ANDROID__

#include "android_wallpaper.h"
#include "renderer.h"
#include "gl_errors.h"
#include <iostream>
#include <unistd.h>



void setupEGLForWallpaper(renderer *rnd, ANativeWindow* window) {
    // Clear any existing EGL context/surface bindings to prevent conflicts with Android's hwuiTask
    // This is critical because Android's WallpaperService may have hardware acceleration enabled
    eglMakeCurrent(EGL_NO_DISPLAY, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);

    // Get EGL display
    rnd->eglDisplay = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (rnd->eglDisplay == EGL_NO_DISPLAY) {
        exit(1);
    }
    
    // Initialize EGL
    EGLint major, minor;
    if (!eglInitialize(rnd->eglDisplay, &major, &minor)) {
        exit(1);
    }

    // Configure EGL attributes without multisampling on the window surface
    // We handle MSAA ourselves with framebuffers to avoid conflicts with Android's UI system
    EGLint attribs[] = {
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_BLUE_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_RED_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 24,
        EGL_NONE
    };
    
    // Choose EGL config
    EGLint numConfigs;
    if (!eglChooseConfig(rnd->eglDisplay, attribs, &rnd->eglConfig, 1, &numConfigs) || numConfigs == 0) {
        exit(1);
    }

    // Create EGL context
    EGLint contextAttribs[] = {
        EGL_CONTEXT_CLIENT_VERSION, 3,
        EGL_NONE
    };
    
    rnd->eglContext = eglCreateContext(rnd->eglDisplay, rnd->eglConfig, EGL_NO_CONTEXT, contextAttribs);
    if (rnd->eglContext == EGL_NO_CONTEXT) {
        exit(1);
    }

    // Set up native window format before creating surface
    // This prevents conflicts with Android's hardware renderer
    ANativeWindow_setBuffersGeometry(window, 0, 0, WINDOW_FORMAT_RGBA_8888);

    // Create EGL surface
    rnd->nativeWindow = window;
    rnd->eglSurface = eglCreateWindowSurface(rnd->eglDisplay, rnd->eglConfig, window, nullptr);
    if (rnd->eglSurface == EGL_NO_SURFACE) {
        EGLint error = eglGetError();

        // The window might be locked by the Canvas operations
        // Try to wait a bit and retry
        usleep(100000); // 100ms

        rnd->eglSurface = eglCreateWindowSurface(rnd->eglDisplay, rnd->eglConfig, window, nullptr);
        if (rnd->eglSurface == EGL_NO_SURFACE) {
            error = eglGetError();
            exit(1);
        }
    }

    // Make context current
    if (!eglMakeCurrent(rnd->eglDisplay, rnd->eglSurface, rnd->eglSurface, rnd->eglContext)) {
        exit(1);
    }
    
    // On Android, OpenGL ES functions are directly available - no GLAD needed
    // The NDK provides GLES3/gl3.h which has all function declarations

    // Get window dimensions
    rnd->opts->width = ANativeWindow_getWidth(window);
    rnd->opts->height = ANativeWindow_getHeight(window);

    // CRITICAL: Set viewport BEFORE any framebuffer operations
    // This establishes the GL state properly and may prevent hwuiTask conflicts
    glViewport(0, 0, rnd->opts->width, rnd->opts->height);

    // Clear the screen to establish the GL context state
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    eglSwapBuffers(rnd->eglDisplay, rnd->eglSurface);

    // CRITICAL: Wait for Android's hwuiTask to fully release the surface
    // The hwuiTask (Hardware UI Task) runs asynchronously and may still be
    // accessing the surface even after we've created our EGL context
    usleep(500000); // 500ms - give hwuiTask time to fully release

    // Do a few more swaps to ensure the GL pipeline is fully established
    for (int i = 0; i < 3; i++) {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        eglSwapBuffers(rnd->eglDisplay, rnd->eglSurface);
        usleep(16666); // ~60fps
    }
}

void android_SwapBuffers(renderer *rnd) {
    if (rnd->eglDisplay != EGL_NO_DISPLAY && rnd->eglSurface != EGL_NO_SURFACE) {
        eglSwapBuffers(rnd->eglDisplay, rnd->eglSurface);
    }
}

void destroyEGL(renderer *rnd) {
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
