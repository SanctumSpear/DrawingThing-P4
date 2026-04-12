#pragma once
#include <glad/glad.h>
#include <string>

class Shader {
public:
    unsigned int programID;

    Shader();
    ~Shader();
    void Use();

private:
    unsigned int CompileShader(unsigned int type, const char* src);
};
