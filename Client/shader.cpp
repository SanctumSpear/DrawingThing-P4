#include "Shader.h"
#include <iostream>

const char* vertexShaderSrc = R"(
    #version 330 core
    layout (location = 0) in vec2 aPos;
    void main() {
        gl_Position = vec4(aPos, 0.0, 1.0);
    }
)";

const char* fragmentShaderSrc = R"(
    #version 330 core
    out vec4 FragColor;
    uniform vec3 uColor;
    void main() {
        FragColor = vec4(uColor, 1.0);
    }
)";

Shader::Shader() {
    unsigned int vert = CompileShader(GL_VERTEX_SHADER, vertexShaderSrc);
    unsigned int frag = CompileShader(GL_FRAGMENT_SHADER, fragmentShaderSrc);

    programID = glCreateProgram();
    glAttachShader(programID, vert);
    glAttachShader(programID, frag);
    glLinkProgram(programID);

    glDeleteShader(vert);
    glDeleteShader(frag);
}

Shader::~Shader() {
    glDeleteProgram(programID);
}

void Shader::Use() {
    glUseProgram(programID);
}

void Shader::SetColor(float r, float g, float b) {
    glUseProgram(programID);
    int location = glGetUniformLocation(programID, "uColor");
    glUniform3f(location, r, g, b);
}

unsigned int Shader::CompileShader(unsigned int type, const char* src) {
    unsigned int shader = glCreateShader(type);
    glShaderSource(shader, 1, &src, nullptr);
    glCompileShader(shader);

    int success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success) {
        char log[512];
        glGetShaderInfoLog(shader, 512, nullptr, log);
        std::cerr << "Shader compile error: " << log << "\n";
    }
    return shader;
}