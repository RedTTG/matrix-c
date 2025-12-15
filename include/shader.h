#ifndef SHADER_H
#define SHADER_H
// ReSharper disable once CppUnusedIncludeDirective
#include <string>
#include <sstream>
#include <array>
#include "glad.h"

std::array<std::stringstream, 2> parseShader(const std::string *source);

std::array<std::stringstream, 2> parseShader(const unsigned char *source, int length);

enum ShaderType {
    NONE = -1,
    VERTEX = 0,
    FRAGMENT = 1
};

class ShaderProgram {
public:
    ShaderProgram();
    ~ShaderProgram() {
        destroy();
    }
    void destroy() const;
    void useProgram() const;
    void linkProgram() const;
    GLuint getUniformLocation(const GLchar *name) const;
    GLuint getUniformBlockIndex(const GLchar *name) const;
    void uniformBlockBinding(GLuint blockIndex, GLuint blockBinding) const;

    // Load individual shader types
    void loadShader(const unsigned char *source, int length, GLuint type);
    void loadShader(const char *source, GLuint type);

    // Parse vertex and fragment shaders from a single source
    void loadShader(const unsigned char *source, int length);


private:
    GLuint program{};
    GLuint vertexShader{};
    GLuint fragmentShader{};
};

#endif //SHADER_H
