#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include "Shader.h"

struct Point { float x, y; };

class Canvas {
public:
    Canvas(int width, int height);
    ~Canvas();

    void Draw();
    void Clear();

    // GLFW callbacks — call these from your static callbacks in main
    void OnMouseButton(int button, int action);
    void OnMouseMove(double mx, double my);
    void OnKeyPress(int key, int action);

private:
    int width, height;
    unsigned int VAO, VBO;
    Shader shader;

    std::vector<std::vector<Point>> strokes;
    std::vector<Point> currentStroke;
    bool mouseDown = false;

    Point ToNDC(double mx, double my);
    void UploadAndDraw(const std::vector<Point>& points);
};