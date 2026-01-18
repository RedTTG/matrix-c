#ifndef TRIANGLE_H
#define TRIANGLE_H
#include <apps.h>
#include <triangle_shader.h>

#ifdef __ANDROID__
#include <GLES3/gl3.h>
#else
#include "glad.h"
#endif

class TriangleApp final : public App {
public:
    explicit TriangleApp(renderer *rnd) : App(rnd) {};
    void setup() override;
    void loop() override;
    void destroy() override;
protected:
    float t = 0;
    int m = 1;
    GLuint vertexArray{};
    GLuint vertexBuffer{};
    GLuint ui_Time{};
    ShaderProgram* program{};
};

#endif //TRIANGLE_H
