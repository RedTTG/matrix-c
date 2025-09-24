#ifndef OPTIONS_H
#define OPTIONS_H
#include <cstdint>
#include <optional>
#include <string>

#include "glad.h"

#include "help_message.h"

enum PostProcessingOptions {
    GHOSTING = 1 << 0,
    BLUR =     1 << 1
};

struct options {
    bool wallpaperMode = false;
    bool fullscreen = true;
    long width = 800;
    long height = 600;
    uint8_t postProcessingOptions = 0;
    uint8_t userAllowedPostProcessingOptions = 0xFF;
    char* app = new char[256];
    GLfloat blurSize = 1.0f;
    GLfloat ghostingBlurSize = 0.0f;
    GLfloat ghostingPreviousFrameOpacity = 0.99f;
    float swapTime = 1.0f / 60.0f;  // Framerate basically
    bool loopWithSwap = true;
    std::optional<std::string> wallpaperImagePath = std::nullopt;

    void maskPostProcessingOptionsWithUserAllowed();
};

options* parseOptions(int argc, char *argv[]);

#endif //OPTIONS_H
