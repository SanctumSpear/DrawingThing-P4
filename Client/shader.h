#pragma once
#include <glad/glad.h>

class Shader {
public:
    unsigned int programID;

    Shader();
    ~Shader();
    void Use();
    void SetColor(float r, float g, float b);

private:
    unsigned int CompileShader(unsigned int type, const char* src);
};