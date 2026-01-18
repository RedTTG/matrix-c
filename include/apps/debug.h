#ifndef DEBUG_H
#define DEBUG_H
#include <apps.h>

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif

static constexpr GLuint indices[] = {
    0, 1, 2, // First triangle
    2, 3, 0 // Second triangle
};

class DebugApp final : public App {
public:
    explicit DebugApp(renderer *rnd) : App(rnd) {};
    void setup() override;
    void loop() override;
    void destroy() override;
private:
    GLfloat vertices[8];
    GLuint vertexArray{};
    GLuint vertexBuffer{};
    GLuint indexBuffer{};
    GLuint ui_MousePosition{};
    ShaderProgram* program{};
};
#endif //DEBUG_H
