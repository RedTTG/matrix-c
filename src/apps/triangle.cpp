#include "apps/triangle.h"

#include <gl_errors.h>

static constexpr GLfloat triangleBufferData[] = {
    -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.5f, 0.0f, 1.0f, 0.0f,
    0.5f, -0.5f, 0.0f, 0.0f, 1.0f
};

void TriangleApp::setup() {
    program = new ShaderProgram();
    rnd->opts->postProcessingOptions |= GHOSTING;
    rnd->opts->postProcessingOptions |= BLUR;
    rnd->opts->blurSize = 0.2f;
    program->loadShader(triangleShader, sizeof(triangleShader));
    program->useProgram();

    GL_CHECK(glGenVertexArrays(1, &vertexArray));
    GL_CHECK(glBindVertexArray(vertexArray));

    GL_CHECK(glGenBuffers(1, &vertexBuffer));
    GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer));
    GL_CHECK(glBufferData(GL_ARRAY_BUFFER, sizeof(triangleBufferData), triangleBufferData, GL_STATIC_DRAW));

    GL_CHECK(glVertexAttribPointer(
            0,
            2,
            GL_FLOAT,
            GL_FALSE,
            5 * sizeof(float),
            nullptr
        ));
    GL_CHECK(glEnableVertexAttribArray(0));
    GL_CHECK(glVertexAttribPointer(
        1,
        3,
        GL_FLOAT,
        GL_FALSE,
        5 * sizeof(float),
        reinterpret_cast<void *>(2 * sizeof(float))
    ));
    GL_CHECK(glEnableVertexAttribArray(1));

    ui_Time = program->getUniformLocation("u_Time");
}

void TriangleApp::loop() {
    program->useProgram();
    t += m * (rnd->clock->deltaTime / 10);

    if (m > 0 && t > 10) {
        m = -1;
    } else if (m < 0 && t < 0) {
        m = 1;
    }

    GL_CHECK(glUniform1f(ui_Time, t));

    GL_CHECK(glBindVertexArray(vertexBuffer));
    GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, 3));
}

void TriangleApp::destroy() {
    GL_CHECK(glDeleteBuffers(1, &vertexBuffer));
    GL_CHECK(glDeleteVertexArrays(1, &vertexArray));
    if (program != nullptr) {
        program->destroy();
        delete program;
    }
}