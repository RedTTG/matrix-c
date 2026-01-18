#include "apps/matrix.h"
#include <fonts.h>
#include <gl_errors.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>

#ifdef __ANDROID__
#include <android/log.h>
#endif

#include "helper.h"
#include "matrix_font.h"
#include "matrix_font_info.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"


void MatrixApp::setup() {
    // Enable post-processing with framerate-independent ghosting
    rnd->opts->postProcessingOptions |= GHOSTING;
    rnd->opts->ghostingPreviousFrameOpacity = 0.97f; // Opacity at 60 FPS (97% retention = long trails)
    rnd->opts->ghostingBlurSize = 0.1f;

    // Handle font initialization
    atlas = createFontTextureAtlas(matrixFont, &matrixFontInfo);

#ifdef __ANDROID__
    int rainLimit = 500;  // Reduced for mobile performance
#else
    int rainLimit = MATRIX_RAIN_LIMIT;
#endif


    // Calculate character scale and mouse radius
    float characterScale = static_cast<float>(rnd->opts->height) / (MATRIX_DEBUG ? 20.0 : 70.0) / static_cast<float>(matrixFontInfo.size);
    mouseRadius = rnd->opts->height / 10.0f;


    // Handle program initialization
    program = new ShaderProgram();
    program->loadShader(matrixVertexShader, sizeof(matrixVertexShader), GL_VERTEX_SHADER);
    // Check the wallpaper image is valid or use rainbow shader
    if (rnd->opts->wallpaperImagePath.has_value() && checkFileExists(rnd->opts->wallpaperImagePath.value())) {
        program->loadShader(matrixFragWallPaperShader, sizeof(matrixFragWallPaperShader), GL_FRAGMENT_SHADER);
        useWallPaperShader = true;
        rnd->opts->ghostingPreviousFrameOpacity = 0.998f;
        rainLimit *= 1.5;
    } else {
        program->loadShader(matrixFragRainbowShader, sizeof(matrixFragRainbowShader), GL_FRAGMENT_SHADER);
    }
    program->linkProgram();
    program->useProgram();

    // Get uniform locations
    GL_CHECK(glUniform1i(program->getUniformLocation("u_AtlasTexture"), 0));
    GL_CHECK(glUniform1i(program->getUniformLocation("u_MaxCharacters"), matrixFontInfo.characterCount-1));
    GL_CHECK(glUniform1i(program->getUniformLocation("u_Rotation"), MATRIX_ROTATION));
    GL_CHECK(glUniform1f(program->getUniformLocation("u_CharacterScaling"), characterScale));
    GL_CHECK(glUniform2f(program->getUniformLocation("u_AtlasTextureSize"), atlas->atlasWidth, atlas->atlasHeight));
    GL_CHECK(glUniform2f(program->getUniformLocation("u_ViewportSize"),
    static_cast<float>(rnd->opts->width),
    static_cast<float>(rnd->opts->height)));

    const GLuint blockIndex = program->getUniformBlockIndex("u_AtlasBuffer");
    program->uniformBlockBinding(blockIndex, 0);

    glm::mat4 projection = glm::ortho(0.0f, static_cast<float>(rnd->opts->width), 0.0f,
                                      static_cast<float>(rnd->opts->height));
    GL_CHECK(glUniformMatrix4fv(program->getUniformLocation("u_Projection"), 1, GL_FALSE, glm::value_ptr(projection)));

    ui_BaseColor = program->getUniformLocation("u_BaseColor");
    ui_Time = program->getUniformLocation("u_Time");

    // Handle vertex buffer initialization
    rainDrawData.resize(rainLimit);
    rainData.resize(rainLimit);

    GL_CHECK(glGenVertexArrays(1, &vertexArray));
    GL_CHECK(glBindVertexArray(vertexArray));

#ifdef __ANDROID__
    // On Android/OpenGL ES, we need actual vertex data for the quad
    // Create a buffer for the quad vertices (4 vertices for TRIANGLE_FAN)
    GLuint quadVertexBuffer;
    float quadVertices[] = {
        0.0f, 0.0f,  // Bottom-left
        1.0f, 0.0f,  // Bottom-right
        1.0f, 1.0f,  // Top-right
        0.0f, 1.0f   // Top-left
    };
    GL_CHECK(glGenBuffers(1, &quadVertexBuffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW));

    // Attribute 3 for quad vertex positions (not instanced)
    GL_CHECK(glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK(glEnableVertexAttribArray(3));
    GL_CHECK(glVertexAttribDivisor(3, 0)); // Not instanced - same for all instances

    // Unbind quad buffer before setting up instance buffer
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif

    // Instance data buffer
    GL_CHECK(glGenBuffers(1, &vertexBuffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL_CHECK(
        glBufferData(GL_ARRAY_BUFFER, rainDrawData.capacity() * sizeof(RainDrawData), nullptr,
            GL_STREAM_DRAW));

    GL_CHECK(glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RainDrawData),
        nullptr
    ));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(
        1,
        1,
        GL_FLOAT,
        GL_FALSE,
        sizeof(RainDrawData),
        reinterpret_cast<void *>(2 * sizeof(float))
    ));
    GL_CHECK(glEnableVertexAttribArray(1));
    GL_CHECK(glVertexAttribPointer(
        2,
        1,
        GL_INT,
        GL_FALSE,
        sizeof(RainDrawData),
        reinterpret_cast<void *>(3 * sizeof(float))
    ));
    GL_CHECK(glEnableVertexAttribArray(2));

    GL_CHECK(glVertexAttribDivisor(0, 1));
    GL_CHECK(glVertexAttribDivisor(1, 1));
    GL_CHECK(glVertexAttribDivisor(2, 1));

#ifdef __ANDROID__
    // Re-bind the quad buffer to attribute 3 to ensure it's set correctly
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, quadVertexBuffer));
    GL_CHECK(glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 0, nullptr));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));
#endif

    // Initialize vertices
    for (int i = 0; i < rainData.capacity(); ++i) {
        if constexpr (MATRIX_DEBUG) {
            rainDrawData[i].x = matrixFontInfo.characterInfoList[i].xOffset * characterScale;
            rainDrawData[i].y = matrixFontInfo.characterInfoList[i].yOffset * characterScale;
        } else {
            rainDrawData[i].x = random_td_float(0, rnd->opts->width);
            rainDrawData[i].y = random_td_float(0, rnd->opts->height);
        }
        rainDrawData[i].colorOffset = randomColorOffset();
        rainDrawData[i].spark = randomSpark();
        rainData[i].speed = randomSpeed();
        if constexpr (MATRIX_DEBUG) {
            rainData[i].speed = 0;
        } else if constexpr (MATRIX_UP) {
            rainData[i].speed *= -1;
        }
    }

    // Load and bind wallpaper texture if needed
    if (useWallPaperShader) {
        int texWidth, texHeight, texChannels;
        unsigned char* imageData = stbi_load(rnd->opts->wallpaperImagePath.value().c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
        if (imageData) {
            GL_CHECK(glGenTextures(1, &wallpaperTexture));
            GL_CHECK(glActiveTexture(GL_TEXTURE1));
            GL_CHECK(glBindTexture(GL_TEXTURE_2D, wallpaperTexture));
            GL_CHECK(glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, texWidth, texHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, imageData));
            GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR));
            GL_CHECK(glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR));
            GL_CHECK(glUniform1i(program->getUniformLocation("u_WallpaperTexture"), 1));
            stbi_image_free(imageData);
        }
    }

}

void MatrixApp::loop() {
    program->useProgram();

#ifdef __ANDROID__
    static int debugFrameCount = 0;
    static bool loggedSetup = false;
    if (!loggedSetup) {
        // Log once to verify loop is being called
        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "Matrix loop() called, capacity=%d", static_cast<int>(rainData.capacity()));

        // Ensure proper GL state for rendering
        glDisable(GL_DEPTH_TEST);
        glDisable(GL_CULL_FACE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "GL state configured");
        loggedSetup = true;
    }
#endif

    // Bind glyph buffer and texture
    GL_CHECK(glActiveTexture(GL_TEXTURE0));
    GL_CHECK(glBindTexture(GL_TEXTURE_2D, atlas->glyphTexture));
    glBindBufferBase(GL_UNIFORM_BUFFER, 0, atlas->glyphBuffer);

    if (useWallPaperShader) {
        GL_CHECK(glActiveTexture(GL_TEXTURE1));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, wallpaperTexture));
    }

    GL_CHECK(glUniform1f(ui_BaseColor, baseColor));
    GL_CHECK(glUniform1f(ui_Time, rnd->clock->floatTime()));

    baseColor += rnd->clock->deltaTime / MATRIX_DELTA_MULTIPLIER;

    int amountOfReassignedRaindrops = std::max(0, static_cast<int>(rnd->events->keysPressed) * MATRIX_EFFECT_PER_KEYPRESS);
    if (rnd->events->mouseLeft) {
        amountOfReassignedRaindrops += MATRIX_DRAW_STRENGTH;
    }

    if (amountOfReassignedRaindrops > rainData.capacity() - activeCursorPardons) {
        amountOfReassignedRaindrops = rainData.capacity() - activeCursorPardons;
    }

    const int reassignedRaindrop = amountOfReassignedRaindrops > 0 ? random_int(0, rainData.capacity() - 1) : -1;

    // Update all rain drops every frame (essential for animation and ghosting trails)
    activeCursorPardons = 0;
    for (int i = 0; i < rainData.capacity(); ++i) {
        incrementRain(i, i == reassignedRaindrop);
        if (rainData[i].cursorPardons > 0) {
            activeCursorPardons++;
        }
    }

    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL_CHECK(glBufferSubData(GL_ARRAY_BUFFER, 0, rainDrawData.size() * sizeof(RainDrawData), rainDrawData.data()));

    // Render
    GL_CHECK(glBindVertexArray(vertexArray));

#ifdef __ANDROID__
    bool shouldLog = debugFrameCount < 3;
    if (shouldLog) {
        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "Frame %d: About to draw %d instances with vertexArray=%u",
            debugFrameCount, static_cast<int>(rainData.capacity()), vertexArray);

        // Log first few instance positions
        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "Frame %d: Instance 0 pos=(%.1f, %.1f), Instance 1 pos=(%.1f, %.1f)",
            debugFrameCount,
            rainDrawData[0].x, rainDrawData[0].y,
            rainDrawData[1].x, rainDrawData[1].y);

        // Check if program is active
        GLint currentProgram = 0;
        glGetIntegerv(GL_CURRENT_PROGRAM, &currentProgram);
        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "Frame %d: Current program ID: %d",
            debugFrameCount, currentProgram);

        // Check VAO state
        GLint vaoBinding = 0;
        glGetIntegerv(GL_VERTEX_ARRAY_BINDING, &vaoBinding);
        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "Frame %d: VAO binding: %d", debugFrameCount, vaoBinding);

        // Check if attribute arrays are enabled
        GLint attr0Enabled = 0, attr1Enabled = 0, attr2Enabled = 0, attr3Enabled = 0;
        glGetVertexAttribiv(0, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr0Enabled);
        glGetVertexAttribiv(1, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr1Enabled);
        glGetVertexAttribiv(2, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr2Enabled);
        glGetVertexAttribiv(3, GL_VERTEX_ATTRIB_ARRAY_ENABLED, &attr3Enabled);
        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "Frame %d: Attrs enabled: 0=%d, 1=%d, 2=%d, 3=%d",
            debugFrameCount, attr0Enabled, attr1Enabled, attr2Enabled, attr3Enabled);

        debugFrameCount++;
    }
#endif

    GL_CHECK(glDrawArraysInstanced(GL_TRIANGLE_FAN, 0, 4, rainData.capacity()));

#ifdef __ANDROID__
    if (shouldLog) {
        __android_log_print(ANDROID_LOG_INFO, "MatrixApp", "Frame %d: Draw call completed", debugFrameCount - 1);
    }
#endif
    // rnd->fboPTextureOutput = atlas->glyphTexture;
}

void MatrixApp::destroy() {
    atlas->destroy();
    GL_CHECK(glDeleteBuffers(1, &vertexBuffer));
    GL_CHECK(glDeleteVertexArrays(1, &vertexArray));
    program->destroy();
}

int MatrixApp::random_int(const int a, const int b) {
    return a + rand() % ((b+1) - a);
}

int MatrixApp::random_td_int(int a, int b)
{
    a /= MATRIX_TEXT_SIZE_DIVISOR;
    b /= MATRIX_TEXT_SIZE_DIVISOR;
    return random_int(a, b) * MATRIX_TEXT_SIZE_DIVISOR;
}

float MatrixApp::random_float(const float a, const float b) {
    return a + static_cast<float>(rand()) / RAND_MAX * (b - a);
}

float MatrixApp::random_td_float(float a, float b) {
    a /= MATRIX_TEXT_SIZE_DIVISOR;
    b /= MATRIX_TEXT_SIZE_DIVISOR;
    return random_float(a, b) * MATRIX_TEXT_SIZE_DIVISOR;
}

int MatrixApp::randomMultiplier() {
    return random_int(0, 1) == 0 ? -1 : 1;
}

int MatrixApp::randomSpark() {
    return random_int(0, MATRIX_CHANCE_OF_SPARK);
}

int MatrixApp::randomSpeed() const {
    return random_td_float(10, 20);
}

float MatrixApp::randomColorOffset() {
    return random_float(-MATRIX_COLOR_VARIATION, MATRIX_COLOR_VARIATION);
}

void MatrixApp::resetRain(const int index) {
    rainDrawData[index].x = random_td_float(0, rnd->opts->width);
    rainDrawData[index].spark = randomSpark();
    rainDrawData[index].colorOffset = randomColorOffset();
    rainData[index].speed = randomSpeed();
    if constexpr (MATRIX_UP) {
        rainData[index].speed *= -1;
    }
}

void MatrixApp::incrementRain(const int index, const bool reassigned) {
    // Move the raindrop along its path
    const float addX = rot_d15_m2 * randomMultiplier();
    const float speed = rainData[index].speed - rot_d15_d2;

    float mouseX = static_cast<float>(rnd->events->mouseX);
    float mouseY = static_cast<float>(rnd->opts->height - rnd->events->mouseY);
    float dx = rainDrawData[index].x - mouseX;
    float dy = rainDrawData[index].y - mouseY;
    float distance = sqrt(dx * dx + dy * dy);

    if (reassigned) {
        rainData[index].cursorPardons = rnd->events->mouseLeft ? 300 : 100;
        rainDrawData[index].colorOffset += 0.5;
        rainDrawData[index].x = mouseX;
        rainDrawData[index].y = mouseY;
        if (random_int(0, 4) == 0) {
            rainData[index].pushX = cos(random_int(0, 360) * M_PI / 180.0f) * rnd->clock->deltaTime * MATRIX_DELTA_MULTIPLIER * MATRIX_SPEED_DRAW;
            rainData[index].pushY = sin(random_int(0, 360) * M_PI / 180.0f) * rnd->clock->deltaTime *  MATRIX_DELTA_MULTIPLIER * MATRIX_SPEED_DRAW;
        } else {
            rainData[index].pushX = 0;
            rainData[index].pushY = 0;
        }
    }

#ifndef __ANDROID__
    if (rainData[index].cursorPardons == 0 and distance < mouseRadius and random_int(0, 10) != 0) {
        // Push the raindrop away from the cursor
        const float force = (mouseRadius - distance) / mouseRadius;
        const float pushX = dx / distance * force * 100.0f; // Adjust the push strength as needed
        const float pushY = dy / distance * force * 100.0f;

        rainDrawData[index].x += pushX;
        rainData[index].pushX += pushX;
        rainData[index].pushY += pushY;
        rainDrawData[index].y += pushY;
    } else
#endif
    if (rainData[index].cursorPardons > 0) {
        // Expand from the cursor
        rainDrawData[index].x += rainData[index].pushX;
        rainDrawData[index].y += rainData[index].pushY;

        rainData[index].cursorPardons--;
        if (rainData[index].cursorPardons == 0) {
            rainData[index].pushX = 0;
            rainData[index].pushY = 0;
        }
    } else if (rainData[index].pushX != 0 || rainData[index].pushY != 0) {
        // Reset the push force
        rainDrawData[index].x -= rainData[index].pushX;
        rainDrawData[index].y -= rainData[index].pushY;
        rainData[index].pushX = 0;
        rainData[index].pushY = 0;
    }

    if (rainData[index].cursorPardons == 0) {
        rainDrawData[index].x += addX;
        rainDrawData[index].y -= speed * rnd->clock->deltaTime * MATRIX_DELTA_MULTIPLIER;
    }

    if (random_int(0, 1000) == 0) {
        rainDrawData[index].x = random_td_float(0, rnd->opts->width);
    }


    // Finally check the position of the raindrop to see if it needs to be reset
    if (MATRIX_UP && rainDrawData[index].y >= rnd->opts->height) {
        rainDrawData[index].y = 0;
        resetRain(index);
    } else if (rainDrawData[index].y < 0) {
        rainDrawData[index].y = static_cast<float>(rnd->opts->height);
        resetRain(index);
    }
}
