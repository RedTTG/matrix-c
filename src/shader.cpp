#include "shader.h"

#include <gl_errors.h>
#include <iostream>
#include <vector>


// Helper function to convert OpenGL shaders to OpenGL ES compatible version
std::string convertShaderForES(const std::string& source) {
#ifdef __ANDROID__
    std::string result = source;
    
    // Constants for version strings
    const char* desktopVersion = "#version 330 core";
    const char* esVersion = "#version 300 es";
    const size_t desktopVersionLen = strlen(desktopVersion);
    
    // Replace #version 330 core with #version 300 es
    size_t versionPos = result.find(desktopVersion);
    if (versionPos != std::string::npos) {
        result.replace(versionPos, desktopVersionLen, esVersion);
        
        // Add precision qualifiers after version directive
        size_t insertPos = versionPos + strlen(esVersion);
        result.insert(insertPos, "\nprecision mediump float;");
    }
    
    return result;
#else
    return source;
#endif
}

std::array<std::stringstream, 2> parseShader(const std::string *source) {
    std::istringstream stream(*source);
    std::string line;
    std::array<std::stringstream, 2> ss;
    ShaderType type = ShaderType::NONE;

    while (getline(stream, line)) {
        if (line.find("#shader") != std::string::npos) {
            if (line.find("vertex") != std::string::npos)
                type = ShaderType::VERTEX;
            else if (line.find("fragment") != std::string::npos)
                type = ShaderType::FRAGMENT;
        } else
            ss[static_cast<int>(type)] << line << '\n';
    }
    return ss;
}

std::array<std::stringstream, 2> parseShader(const unsigned char *source, const int length) {
    const std::string str(reinterpret_cast<const char*>(source), length);
    return parseShader(&str);
}

ShaderProgram::ShaderProgram() {
    GL_CHECK(program = glCreateProgram());
}

void ShaderProgram::destroy() const {
    // Detach the shaders
    GL_CHECK(glDetachShader(program, vertexShader));
    GL_CHECK(glDetachShader(program, fragmentShader));

    // Delete the shaders
    GL_CHECK(glDeleteShader(vertexShader));
    GL_CHECK(glDeleteShader(fragmentShader));

    // Delete the program
    GL_CHECK(glDeleteProgram(program));
}

void ShaderProgram::useProgram() const {
    GL_CHECK(glUseProgram(program));
}

void ShaderProgram::linkProgram() const {
    GL_CHECK(glLinkProgram(program));

    // Check the program
    GLint Result = GL_FALSE;
    int InfoLogLength;

    GL_CHECK(glGetProgramiv(program, GL_LINK_STATUS, &Result));
    GL_CHECK(glGetProgramiv(program, GL_INFO_LOG_LENGTH, &InfoLogLength));

    if (InfoLogLength > 0) {
        std::vector<char> ProgramErrorMessage(InfoLogLength + 1);
        GL_CHECK(glGetProgramInfoLog(program, InfoLogLength, nullptr, &ProgramErrorMessage[0]));
        std::cerr << "Shader link log: " << &ProgramErrorMessage[0] << std::endl;
    }

    if (Result == GL_FALSE) {
        std::cerr << "Shader program linking failed!" << std::endl;
    }
}


GLuint ShaderProgram::getUniformLocation(const GLchar *name) const {
    return glGetUniformLocation(program, name);
}

GLuint ShaderProgram::getUniformBlockIndex(const GLchar *name) const {
    return glGetUniformBlockIndex(program, name);
}

void ShaderProgram::uniformBlockBinding(const GLuint blockIndex, const GLuint blockBinding) const {
    GL_CHECK(glUniformBlockBinding(program, blockIndex, blockBinding));
}

void ShaderProgram::loadShader(const unsigned char *source, const int length, const GLuint type) {
    const std::string src(reinterpret_cast<const char *>(source), length);
    const std::string convertedSrc = convertShaderForES(src);
    return loadShader(convertedSrc.c_str(), type);
}

void ShaderProgram::loadShader(const char *source, const GLuint type) {
    // Convert shader for OpenGL ES if needed
    const std::string convertedSource = convertShaderForES(std::string(source));
    const char* finalSource = convertedSource.c_str();
    
    // Create a new openGL shader
    const GLuint shader = glCreateShader(type);

    // Compile the shader from source
    GL_CHECK(glShaderSource(shader, 1, &finalSource, nullptr));
    GL_CHECK(glCompileShader(shader));

    // Check for compilation errors
    GLint success;
    GL_CHECK(glGetShaderiv(shader, GL_COMPILE_STATUS, &success));
    if (!success) {
        GLint logLength;
        GL_CHECK(glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength));
        std::vector<char> log(logLength);
        GL_CHECK(glGetShaderInfoLog(shader, logLength, &logLength, log.data()));
        std::cerr << "Shader compilation failed (type=" << type << "): " << log.data() << std::endl;
        std::cerr << "Shader source (first 500 chars): " << std::string(finalSource).substr(0, 500) << std::endl;
    }

    // Attach the shader to the program
    GL_CHECK(glAttachShader(program, shader));

    // Store the shader for detaching later
    if (type == GL_VERTEX_SHADER)
        vertexShader = shader;
    else if (type == GL_FRAGMENT_SHADER)
        fragmentShader = shader;
}

void ShaderProgram::loadShader(const unsigned char *source, const int length) {
    const std::array<std::stringstream, 2> sources = parseShader(source, length);
    const std::string vertexSource = sources[0].str();
    const std::string fragmentSource = sources[1].str();

    loadShader(vertexSource.c_str(), GL_VERTEX_SHADER);
    loadShader(fragmentSource.c_str(), GL_FRAGMENT_SHADER);

    linkProgram();
}

