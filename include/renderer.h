#ifndef RENDERER_H
#define RENDERER_H
#include <apps.h>
#include <clock.h>
#include <options.h>
#include <events.h>
#include <iostream>
#include <shader.h>

#ifdef __ANDROID__
#include <EGL/egl.h>
#include <GLES3/gl3.h>
#include <android/native_window.h>
#else
#include "glad.h"
#include <GL/gl.h>
#include <GLFW/glfw3.h>
#endif

#if defined(__linux__) && !defined(__ANDROID__)
#include <X11/Xlib.h>
#include <X11/extensions/Xrender.h>
#include <GL/glx.h>
#endif



#define TITLE "Matrix rain"

#if defined(__linux__) && !defined(__ANDROID__)
typedef GLXContext (*glXCreateContextAttribsARBProc)(Display *, GLXFBConfig, GLXContext, Bool, const int *);
#endif

static constexpr GLfloat ppFullQuadBufferData[] = {
    // Coordinates    // Texture coordinates
     1.0f, -1.0f,     1.0f, 0.0f,
    -1.0f, -1.0f,     0.0f, 0.0f,
    -1.0f,  1.0f,     0.0f, 1.0f,

     1.0f,  1.0f,     1.0f, 1.0f,
     1.0f, -1.0f,     1.0f, 0.0f,
    -1.0f,  1.0f,     0.0f, 1.0f
};

struct renderer {
    options *opts;
#if defined(__linux__) && !defined(__ANDROID__)
    Display *display = nullptr;
    Window desktop{};
    Window window{};
    Window root{};
    GLXContext ctx{};
    int screenNum = -1;
    GLXFBConfig *fbcs = nullptr;
    XVisualInfo *vi = nullptr;
    XRenderPictFormat *pict = nullptr;
    GLXFBConfig fbc{};
    XSetWindowAttributes swa{};
    bool x11 = false;
    bool x11MouseEvents = false;
    int xinputOptCode{};
    static void handleSignal(int signal);
    void setupSignalHandling();
#elif defined(__ANDROID__)
    EGLDisplay eglDisplay = EGL_NO_DISPLAY;
    EGLContext eglContext = EGL_NO_CONTEXT;
    EGLSurface eglSurface = EGL_NO_SURFACE;
    EGLConfig eglConfig;
    ANativeWindow* nativeWindow = nullptr;
    bool androidEGL = false;
#else

#endif
    int antialiasSamples = 4;
    App *app = nullptr;
    tickRateClock *clock;
#ifndef __ANDROID__
    GLFWwindow *glfwWindow = nullptr;
#endif

    ShaderProgram *ppGhostingProgram{};
    ShaderProgram *ppBlurProgram{};

    ShaderProgram *ppFinalProgram{};

    GLuint ppFullQuadArray{};
    GLuint ppFullQuadBuffer{};

    GLuint fboC{};
    GLuint fboCTexture{};
    GLuint fboM{};
    GLuint fboMTexture{};
    GLuint fboP{};
    GLuint fboPTexture{};

    GLuint fboCOutput{};
    GLuint fboCTextureOutput{};
    GLuint fboMOutput{};
    GLuint fboMTextureOutput{};
    GLuint fboPOutput{};
    GLuint fboPTextureOutput{};
    GLuint RBO{};

    void makeWindow();

    void getEvents() const;

    void loadApp();
    void loopApp() const;
    void destroyApp() const;

    void frameBegin() const;

    static void clear();

    void _swapPPBuffersCM();

    void _swapPPBuffersPM();

    void _resolveMultisampledFramebuffer(GLuint srcFbo, GLuint dstFbo) const;

    void _sampleFrameBuffersForPostProcessing() const;

    void frameEnd();

    groupedEvents *events = nullptr;

    static renderer *instance;

    explicit renderer(options *opts);

    void makeContext();

    void makeFrameBuffers();
    void createFrameBufferTexture(GLuint &fbo, GLuint &fboTexture, GLuint format, bool multiSampled) const;

    void initializePP();

    void initialize();

    void swapBuffers();
    void destroy() const;
};

#endif //RENDERER_H
