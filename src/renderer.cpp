#include "renderer.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <fonts.h>
#include <gl_errors.h>
#include <shader.h>
#include <vector>
#include "basic_texture_fragment_shader.h"
#include "basic_texture_vertex_shader.h"
#include "ghosting_fragment_shader.h"
#include "blur_fragment_shader.h"

#if defined(__linux__) && !defined(__ANDROID__)
#include "x11.h"
#include <csignal>
#include <signal.h>

auto glXCreateContextAttribsARB = reinterpret_cast<glXCreateContextAttribsARBProc>(
    glXGetProcAddressARB(
        reinterpret_cast<const GLubyte *>("glXCreateContextAttribsARB")));
#endif

#ifdef __ANDROID__
#include "android_wallpaper.h"
#endif

renderer *renderer::instance = nullptr;

renderer::renderer(options *opts) {
    this->opts = opts;
    clock = new tickRateClock();
    events = new groupedEvents();
    instance = this;
}

#ifndef __ANDROID__
void initializeGlad() {
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glfwGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        exit(1);
    }
}
#endif

#if defined(__linux__) && !defined(__ANDROID__)

void glXInitializeGlad() {
    if (!gladLoadGLLoader(reinterpret_cast<GLADloadproc>(glXGetProcAddress))) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        exit(1);
    }
}

void renderer::handleSignal(const int signal) {
    if (signal == SIGINT || signal == SIGTERM || signal == SIGSTOP) {
        instance->events->quit = true;
    }
}

void renderer::setupSignalHandling() {
    const auto handler = handleSignal;
    std::signal(SIGINT, handler);
    std::signal(SIGTERM, handler);
    std::signal(SIGSTOP, handler);
}
#endif

void renderer::makeContext() {
    if (opts->wallpaperMode) {
#if defined(__linux__) && !defined(__ANDROID__)
        display = XOpenDisplay(nullptr);
        setupWindowForWallpaperMode(this);
        int gl3attr[] = {
            GLX_CONTEXT_MAJOR_VERSION_ARB, 3,
            GLX_CONTEXT_MINOR_VERSION_ARB, 0,
            GLX_CONTEXT_PROFILE_MASK_ARB, GLX_CONTEXT_CORE_PROFILE_BIT_ARB,
            GLX_CONTEXT_FLAGS_ARB, GLX_CONTEXT_FORWARD_COMPATIBLE_BIT_ARB,
            None
        };

        ctx = glXCreateContextAttribsARB(display, fbc, nullptr, True, gl3attr);

        if (!ctx) {
            printf("Couldn't create an OpenGL context\n");
            exit(1);
        }

        XTextProperty windowName;
        windowName.value = reinterpret_cast<unsigned char *>(const_cast<char *>(TITLE));
        windowName.encoding = XA_STRING;
        windowName.format = 8;
        windowName.nitems = strlen(reinterpret_cast<char *>(windowName.value));

        XSetWMName(display, window, &windowName);

        XMapWindow(display, window);

        int event, error;
        if (!XQueryExtension(display, "XInputExtension", &xinputOptCode, &event, &error)) {
            std::cerr << "X Input extension not available" << std::endl;
            XSelectInput(
                display, window,
                ExposureMask |
                KeyPressMask | KeyReleaseMask |
                StructureNotifyMask |
                ButtonPressMask | ButtonReleaseMask | PointerMotionMask
            );
        } else {
            x11MouseEvents = true;
            XIEventMask evmask;
            unsigned char mask[(XI_LASTEVENT + 7) / 8] = {0};
            evmask.deviceid = XIAllDevices;
            evmask.mask_len = sizeof(mask);
            evmask.mask = mask;
            XISetMask(mask, XI_RawMotion);
            XISetMask(mask, XI_RawKeyPress);
            XISetMask(mask, XI_RawKeyRelease);
            XISetMask(mask, XI_RawButtonPress);
            XISetMask(mask, XI_RawButtonRelease);

            XISelectEvents(this->display, DefaultRootWindow(this->display), &evmask, 1);
        }

        glXMakeCurrent(display, window, ctx);

        glXInitializeGlad();

        GL_CHECK(glViewport(0, 0, opts->width, opts->height));

        x11 = true;

#elif defined(__ANDROID__)
        // Android EGL setup
        setupEGLForWallpaper(this, nativeWindow);
        GL_CHECK(glViewport(0, 0, opts->width, opts->height));
        androidEGL = true;

#else
        std::cerr << "Wallpaper mode is only supported on Linux and Android" << std::endl;
        exit(1);
#endif
        return;
    }
    
#if !defined(__ANDROID__)
    // Create a normal GLFW context

    if (!glfwInit()) {
        std::cerr << "Couldn't initialize GLFW" << std::endl;
        exit(1);
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 2);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_SAMPLES, antialiasSamples);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
    if (opts->fullscreen) {
        GLFWmonitor *primaryMonitor = glfwGetPrimaryMonitor();
        const GLFWvidmode *mode = glfwGetVideoMode(primaryMonitor);
        glfwWindow = glfwCreateWindow(mode->width, mode->height, TITLE, primaryMonitor, nullptr);
        opts->width = mode->width;
        opts->height = mode->height;
    } else {
        glfwWindow = glfwCreateWindow(opts->width, opts->height, TITLE, nullptr, nullptr);
    }

    if (!glfwWindow) {
        const char *description;
        int code = glfwGetError(&description);
        std::cerr << "Couldn't create a GLFW window: " << description << " (Error code: " << code << ")" << std::endl;
        glfwTerminate();
        exit(1);
    }

    glfwMakeContextCurrent(glfwWindow);

    initializeGlad();
#else
    std::cerr << "Non-wallpaper mode is not supported on Android" << std::endl;
    exit(1);
#endif
}

void renderer::makeFrameBuffers() {
#ifdef __ANDROID__
    // On Android, we cannot create framebuffers due to hwuiTask conflicts
    // Instead, render directly to screen (FBO 0) and implement ghosting differently:
    // - Don't clear the screen completely each frame
    // - Use glBlendFunc to create trailing effect

    fboC = 0;
    fboM = 0;
    fboP = 0;
    fboCOutput = 0;
    fboMOutput = 0;
    fboPOutput = 0;

    fboCTexture = 0;
    fboMTexture = 0;
    fboPTexture = 0;
    fboCTextureOutput = 0;
    fboMTextureOutput = 0;
    fboPTextureOutput = 0;

    RBO = 0;

    return;
#else
    // Desktop uses multisampling
    GLint maxSamples;
    GL_CHECK(glGetIntegerv(GL_MAX_SAMPLES, &maxSamples));

    if (antialiasSamples > maxSamples) {
        std::cerr << "Requested " << antialiasSamples << " samples but GL supports max " << maxSamples << ", clamping" << std::endl;
        antialiasSamples = maxSamples;
    }

    createFrameBufferTexture(fboC, fboCTexture, GL_RGBA8, true);
    createFrameBufferTexture(fboM, fboMTexture, GL_RGBA8, true);
    createFrameBufferTexture(fboP, fboPTexture, GL_RGBA8, true);

    createFrameBufferTexture(fboCOutput, fboCTextureOutput, GL_RGBA8, false);
    createFrameBufferTexture(fboMOutput, fboMTextureOutput, GL_RGBA8, false);
    createFrameBufferTexture(fboPOutput, fboPTextureOutput, GL_RGBA8, false);

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));

    // Create renderbuffer for depth/stencil
    GL_CHECK(glGenRenderbuffers(1, &RBO));
    GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, RBO));
    GL_CHECK(
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, antialiasSamples, GL_DEPTH24_STENCIL8, opts->width, opts->
            height));

    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fboC));
    GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, RBO));
    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete" << std::endl;
        exit(1);
    }

    // Unbind the framebuffer
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
#endif
}

void renderer::createFrameBufferTexture(GLuint &fbo, GLuint &fboTexture, const GLuint format,
                                        const bool multiSampled) const {
#ifdef __ANDROID__
    GL_CHECK(glGenFramebuffers(1, &fbo));
#else
    GL_CHECK(glCreateFramebuffers(1, &fbo));
#endif
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fbo));

    if (multiSampled) {
#ifdef __ANDROID__
        // OpenGL ES 3.0: Use renderbuffer for multisampling instead of multisampled textures
        GL_CHECK(glGenRenderbuffers(1, &fboTexture));
        GL_CHECK(glBindRenderbuffer(GL_RENDERBUFFER, fboTexture));
        GL_CHECK(glRenderbufferStorageMultisample(GL_RENDERBUFFER, antialiasSamples, format, opts->width, opts->height));
        GL_CHECK(glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, fboTexture));

        // Verify framebuffer is complete
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE) {
            // Framebuffer incomplete - silently fail on Android
        }
#else
        // Desktop OpenGL: Use multisampled textures
        GL_CHECK(glGenTextures(1, &fboTexture));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fboTexture));
        GL_CHECK(
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, antialiasSamples, format, opts->width, opts->height,
                GL_TRUE));
        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, fboTexture, 0));
#endif
    } else {
        GL_CHECK(glGenTextures(1, &fboTexture));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, fboTexture));

        // Determine the format for the texture data (not internal format)
        // GL_RGBA8 is an internal format, but glTexImage2D expects GL_RGBA for the format parameter
        GLuint dataFormat = (format == GL_RGBA8) ? GL_RGBA : format;

        GL_CHECK(
            glTexImage2D(GL_TEXTURE_2D, 0, format, opts->width, opts->height, 0, dataFormat, GL_UNSIGNED_BYTE, nullptr));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL_CHECK(glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, fboTexture, 0));
    }

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        std::cerr << "Framebuffer is not complete" << std::endl;
        exit(1);
    }
}

void renderer::initializePP() {
#ifdef __ANDROID__
    // Create a simple quad for drawing fade overlay
    GL_CHECK(glGenVertexArrays(1, &ppFullQuadArray));
    GL_CHECK(glBindVertexArray(ppFullQuadArray));

    GL_CHECK(glGenBuffers(1, &ppFullQuadBuffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, ppFullQuadBuffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(ppFullQuadBufferData), ppFullQuadBufferData, GL_STATIC_DRAW));

    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr));
    GL_CHECK(glEnableVertexAttribArray(0));

    // Create a simple shader program for drawing solid color quad
    const char* fadeVertexShader = R"(
        #version 300 es
        layout(location = 0) in vec2 position;
        void main() {
            gl_Position = vec4(position, 0.0, 1.0);
        }
    )";

    const char* fadeFragmentShader = R"(
        #version 300 es
        precision mediump float;
        uniform float u_alpha;
        out vec4 fragColor;
        void main() {
            fragColor = vec4(0.0, 0.0, 0.0, u_alpha);
        }
    )";

    ppFinalProgram = new ShaderProgram();
    ppFinalProgram->loadShader(fadeVertexShader, GL_VERTEX_SHADER);
    ppFinalProgram->loadShader(fadeFragmentShader, GL_FRAGMENT_SHADER);
    ppFinalProgram->linkProgram();

    return;
#endif

    // Desktop: Full post-processing setup
    // Create VAO for full-screen quad
    GL_CHECK(glGenVertexArrays(1, &ppFullQuadArray));
    GL_CHECK(glBindVertexArray(ppFullQuadArray));

    GL_CHECK(glGenBuffers(1, &ppFullQuadBuffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, ppFullQuadBuffer));

    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(ppFullQuadBufferData), ppFullQuadBufferData, GL_STATIC_DRAW));

    GL_CHECK(glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), nullptr));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(GLfloat), (void *)(2 * sizeof(GLfloat))));
    GL_CHECK(glEnableVertexAttribArray(1));

    // Create the final post-processing program
    ppFinalProgram = new ShaderProgram();
    ppFinalProgram->loadShader(basicTextureVertexShader, sizeof(basicTextureVertexShader), GL_VERTEX_SHADER);
    ppFinalProgram->loadShader(basicTextureFragmentShader, sizeof(basicTextureFragmentShader), GL_FRAGMENT_SHADER);
    ppFinalProgram->linkProgram();

    // Create option specific post-processing programs
    if (opts->postProcessingOptions & GHOSTING) {
        ppGhostingProgram = new ShaderProgram();
        ppGhostingProgram->loadShader(basicTextureVertexShader, sizeof(basicTextureVertexShader), GL_VERTEX_SHADER);
        ppGhostingProgram->loadShader(ghostingFragmentShader, sizeof(ghostingFragmentShader), GL_FRAGMENT_SHADER);
        ppGhostingProgram->linkProgram();
    }
    if (opts->postProcessingOptions & (GHOSTING | BLUR)) {
        ppBlurProgram = new ShaderProgram();
        ppBlurProgram->loadShader(basicTextureVertexShader, sizeof(basicTextureVertexShader), GL_VERTEX_SHADER);
        ppBlurProgram->loadShader(blurFragmentShader, sizeof(blurFragmentShader), GL_FRAGMENT_SHADER);
        ppBlurProgram->linkProgram();
    }

    GL_CHECK(glEnable(GL_BLEND));
    GL_CHECK(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));
}

void renderer::initialize() {
#if defined(__linux__) && !defined(__ANDROID__)
    setupSignalHandling();
#endif
    makeContext();
    makeFrameBuffers();
    clock->initialize();
    loadApp();
    opts->maskPostProcessingOptionsWithUserAllowed();
    initializePP();
}

void renderer::swapBuffers() {
    GL_CHECK(glFlush());
#if defined(__linux__) && !defined(__ANDROID__)
    if (x11) {
        x11_SwapBuffers(this);
        return;
    }
#elif defined(__ANDROID__)
    if (androidEGL) {
        android_SwapBuffers(this);
        return;
    }
#endif
#if !defined(__ANDROID__)
    glfwSwapBuffers(glfwWindow);
#endif
}

void renderer::destroy() const {
    // Destroy app first if it exists (while context is still valid)
    if (app != nullptr) {
        try {
            destroyApp();
        } catch (...) {
            // Catch any exceptions during app cleanup
        }
    }

    // CRITICAL: Delete OpenGL resources BEFORE destroying the EGL context
    // Delete the framebuffers
    try {
        GL_CHECK(glDeleteFramebuffers(1, &fboC));
        GL_CHECK(glDeleteTextures(1, &fboCTexture));
        GL_CHECK(glDeleteFramebuffers(1, &fboM));
        GL_CHECK(glDeleteTextures(1, &fboMTexture));
        GL_CHECK(glDeleteFramebuffers(1, &fboP));
        GL_CHECK(glDeleteTextures(1, &fboPTexture));

        GL_CHECK(glDeleteFramebuffers(1, &fboCOutput));
        GL_CHECK(glDeleteTextures(1, &fboCTextureOutput));
        GL_CHECK(glDeleteFramebuffers(1, &fboMOutput));
        GL_CHECK(glDeleteTextures(1, &fboMTextureOutput));
        GL_CHECK(glDeleteFramebuffers(1, &fboPOutput));
        GL_CHECK(glDeleteTextures(1, &fboPTextureOutput));

        GL_CHECK(glDeleteRenderbuffers(1, &RBO));
        GL_CHECK(glDeleteVertexArrays(1, &ppFullQuadArray));
        GL_CHECK(glDeleteBuffers(1, &ppFullQuadBuffer));

        if (ppFinalProgram != nullptr) {
            ppFinalProgram->destroy();
        }
        if (opts->postProcessingOptions & GHOSTING && ppGhostingProgram != nullptr) {
            ppGhostingProgram->destroy();
        }
        if (opts->postProcessingOptions & BLUR && ppBlurProgram != nullptr) {
            ppBlurProgram->destroy();
        }
    } catch (...) {
        // Catch any exceptions during OpenGL cleanup
    }

    // NOW destroy the window/context after OpenGL cleanup is done
#if defined(__linux__) && !defined(__ANDROID__)
    if (x11) {
        XCloseDisplay(display);
    }
#elif defined(__ANDROID__)
    if (androidEGL) {
        destroyEGL(const_cast<renderer*>(this));
    }
#else
    if constexpr (false) {}
#endif
#if !defined(__ANDROID__)
    else {
        glfwDestroyWindow(glfwWindow);
        glfwTerminate();
    }
#endif

    delete clock;
    delete events;
    delete opts;
}

void renderer::getEvents() const {
    clock->calculateFrameSwapDeltaTime();
#if defined(__linux__) && !defined(__ANDROID__)
    if (x11) {
        handleX11Events(this);
        return;
    }
#elif defined(__ANDROID__)
    if (androidEGL) {
        // Android events handled via JNI callbacks
        // No polling needed here
        return;
    }
#endif
#if !defined(__ANDROID__)
    handleGLFWEvents(this);
#endif
}

void renderer::loadApp() {
    app = initializeApp(this, opts->app);
}

void renderer::loopApp() const {
    app->loop();
}

void renderer::destroyApp() const {
    app->destroy();
    delete app;
}

void renderer::frameBegin() const {
    clock->calculateDeltaTime();
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fboC));

#ifdef __ANDROID__
    // Implement ghosting on Android using fade overlay
    if (opts->postProcessingOptions & GHOSTING) {
        static bool firstFrame = true;

        // Only clear the very first frame
        if (firstFrame) {
            clear();
            firstFrame = false;
            return;
        }

        // Draw a fade overlay to darken the previous frame
        // More aggressive fade for more visible trails
        float fadeAlpha = 0.08f; // 8% fade per frame = 92% retention

        GL_CHECK(glDisable(GL_DEPTH_TEST));
        GL_CHECK(glDisable(GL_CULL_FACE));
        GL_CHECK(glEnable(GL_BLEND));
        GL_CHECK(glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));

        ppFinalProgram->useProgram();
        GL_CHECK(glUniform1f(ppFinalProgram->getUniformLocation("u_alpha"), fadeAlpha));
        GL_CHECK(glBindVertexArray(ppFullQuadArray));
        GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));


        // CRITICAL: Restore the blend mode that matrix app expects
        // This prevents flicker by ensuring consistent blending
        GL_CHECK(glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA));

        return;
    }
#endif

    clear();
}

void renderer::clear() {
#ifdef __ANDROID__
    GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 1.0f));
#else
    GL_CHECK(glClearColor(0.0f, 0.0f, 0.0f, 0.0f));
#endif
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT));
}

void renderer::_swapPPBuffersCM() {
    GLuint temp = fboCTexture;
    fboCTexture = fboMTexture;
    fboMTexture = temp;

    temp = fboC;
    fboC = fboM;
    fboM = temp;

    temp = fboCTextureOutput;
    fboCTextureOutput = fboMTextureOutput;
    fboMTextureOutput = temp;

    temp = fboCOutput;
    fboCOutput = fboMOutput;
    fboMOutput = temp;
}

void renderer::_swapPPBuffersPM() {
    GLuint temp = fboPTexture;
    fboPTexture = fboMTexture;
    fboMTexture = temp;

    temp = fboP;
    fboP = fboM;
    fboM = temp;

    temp = fboPTextureOutput;
    fboPTextureOutput = fboMTextureOutput;
    fboMTextureOutput = temp;

    temp = fboPOutput;
    fboPOutput = fboMOutput;
    fboMOutput = temp;
}

void renderer::_resolveMultisampledFramebuffer(const GLuint srcFbo, const GLuint dstFbo) const {
#ifdef __ANDROID__
    // On Android without multisampling, only blit if different
    if (srcFbo != dstFbo) {
        GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo));
        GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo));
        GL_CHECK(
            glBlitFramebuffer(0, 0, opts->width, opts->height, 0, 0, opts->width, opts->height, GL_COLOR_BUFFER_BIT,
                GL_NEAREST));
    }
#else
    // Desktop with multisampling - always blit to resolve
    GL_CHECK(glBindFramebuffer(GL_READ_FRAMEBUFFER, srcFbo));
    GL_CHECK(glBindFramebuffer(GL_DRAW_FRAMEBUFFER, dstFbo));
    GL_CHECK(
        glBlitFramebuffer(0, 0, opts->width, opts->height, 0, 0, opts->width, opts->height, GL_COLOR_BUFFER_BIT,
            GL_NEAREST));
#endif
}

void renderer::_sampleFrameBuffersForPostProcessing() const {
    _resolveMultisampledFramebuffer(fboC, fboCOutput);
    _resolveMultisampledFramebuffer(fboP, fboPOutput);
}

void renderer::frameEnd() {
#ifdef __ANDROID__
    // On Android, ghosting is handled in frameBegin by fading with glClear opacity
    // No FBO-based post-processing needed here
    return;
#endif

    // Desktop: Full post-processing with framebuffers
    // Handle post-processing
    if (opts->postProcessingOptions & GHOSTING) {
        _sampleFrameBuffersForPostProcessing();

        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fboM));
        clear();  // Clear destination framebuffer before rendering ghosting blend into it
        ppGhostingProgram->useProgram();

        GL_CHECK(glUniform1i(ppGhostingProgram->getUniformLocation("u_textureC"), 0));
        GL_CHECK(glUniform1i(ppGhostingProgram->getUniformLocation("u_textureP"), 1));
#ifdef __ANDROID__
        GL_CHECK(
            glUniform1f(ppGhostingProgram->getUniformLocation("u_previousFrameOpacity"), opts->
                ghostingPreviousFrameOpacity));
#else
        // Calculate framerate-independent opacity
        // ghostingPreviousFrameOpacity is the base opacity at 60 FPS (e.g., 0.97 = 97% retention per frame)
        // Scale it based on actual frame time to maintain consistent visual decay rate
        float targetFPS = 60.0f;
        float frameTimeRatio = (clock->deltaTime * targetFPS);
        // Use power function to maintain exponential decay rate across different framerates
        float frameOpacity = pow(opts->ghostingPreviousFrameOpacity, frameTimeRatio);

        GL_CHECK(glUniform1f(ppGhostingProgram->getUniformLocation("u_previousFrameOpacity"), frameOpacity));
#endif
        GL_CHECK(glBindVertexArray(ppFullQuadArray));

        // Bind the framebuffer textures
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, fboCTextureOutput));

        GL_CHECK(glActiveTexture(GL_TEXTURE1));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, fboPTextureOutput));

        GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));

        _swapPPBuffersCM();
        if (opts->ghostingBlurSize > 0.0f) {
            GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fboM));
            ppBlurProgram->useProgram();

            GL_CHECK(glUniform1i(ppBlurProgram->getUniformLocation("u_textureC"), 0));
            GL_CHECK(glUniform1f(ppBlurProgram->getUniformLocation("u_blurSize"), opts->ghostingBlurSize));
            GL_CHECK(glBindVertexArray(ppFullQuadArray));

            // Bind the framebuffer textures
            GL_CHECK(glActiveTexture(GL_TEXTURE0));
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, fboPTextureOutput));

            GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));

            _swapPPBuffersPM();
        }
    }
    if (opts->postProcessingOptions & BLUR) {
        _sampleFrameBuffersForPostProcessing();
        GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, fboM));
        clear();
        ppBlurProgram->useProgram();

        GL_CHECK(glUniform1i(ppBlurProgram->getUniformLocation("u_textureC"), 0));
        GL_CHECK(glUniform1f(ppBlurProgram->getUniformLocation("u_blurSize"), opts->blurSize));
        GL_CHECK(glBindVertexArray(ppFullQuadArray));

        // Bind the framebuffer textures
        GL_CHECK(glActiveTexture(GL_TEXTURE0));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, fboCTextureOutput));

        GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));

        _swapPPBuffersCM();
    }
    _resolveMultisampledFramebuffer(fboC, fboCOutput);
    GL_CHECK(glBindFramebuffer(GL_FRAMEBUFFER, 0));
    clear(); // This is correct btw
    ppFinalProgram->useProgram();
    GL_CHECK(glBindVertexArray(ppFullQuadArray));

    GL_CHECK(glUniform1i(ppFinalProgram->getUniformLocation("u_texture"), 0));

    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, fboCTextureOutput));
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 6));

    // Swap the framebuffers
    if (clock->frameSwapDeltaTime >= opts->swapTime) {
        GLuint temp = fboPTexture;
        fboPTexture = fboCTexture;
        fboCTexture = temp;

        temp = fboP;
        fboP = fboC;
        fboC = temp;

        temp = fboPTextureOutput;
        fboPTextureOutput = fboCTextureOutput;
        fboCTextureOutput = temp;

        temp = fboPOutput;
        fboPOutput = fboCOutput;
        fboCOutput = temp;
        clock->resetFrameSwapTime();
    }
}
