#include "apps/debug.h"

#include <gl_errors.h>

#include "cursor_motion_vertex_shader.h"
#include "debug_fragment_shader.h"
#include <helper.h>

void DebugApp::setup() {
    // Enable ghosting post-processing effect
    rnd->opts->postProcessingOptions = 0xFF;
    rnd->opts->blurSize = 2.0f;
    createQuadVertexData(rnd, 50.0, 50.0, vertices);

    program = new ShaderProgram();
    program->loadShader(debugFragmentShader, sizeof(debugFragmentShader), GL_FRAGMENT_SHADER);
    program->loadShader(cursorMotionVertexShader, sizeof(cursorMotionVertexShader), GL_VERTEX_SHADER);
    program->linkProgram();
    program->useProgram();

    GL_CHECK(glUniform2f(
        program->getUniformLocation("u_ScreenSize"),
        static_cast<GLfloat>(rnd->opts->width),
        static_cast<GLfloat>(rnd->opts->height)
    ));
    ui_MousePosition = program->getUniformLocation("u_MousePosition");

    GL_CHECK(glGenVertexArrays(1, &vertexArray));
    GL_CHECK(glBindVertexArray(vertexArray));

    GL_CHECK(glGenBuffers(1, &vertexBuffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW));

    GL_CHECK(glVertexAttribPointer(
        0,
        2,
        GL_FLOAT,
        GL_FALSE,
        0,
        nullptr
    ));
    GL_CHECK(glEnableVertexAttribArray(0));

    GL_CHECK(glGenBuffers(1, &indexBuffer));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));
    GL_CHECK(glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW));
}

void DebugApp::loop() {
    program->useProgram();
    GL_CHECK(glUniform2f(
        ui_MousePosition,
        static_cast<GLfloat>(rnd->events->mouseX),
        static_cast<GLfloat>(rnd->events->mouseY)
    ));

    GL_CHECK(glBindVertexArray(vertexArray));
    GL_CHECK(glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer));

    GL_CHECK(glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr));
}

void DebugApp::destroy() {
    GL_CHECK(glDeleteBuffers(1, &vertexBuffer));
    GL_CHECK(glDeleteBuffers(1, &indexBuffer));
    GL_CHECK(glDeleteVertexArrays(1, &vertexArray));
    if (program != nullptr) {
        program->destroy();
        delete program;
    }
}
