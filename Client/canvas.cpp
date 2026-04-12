#include "Canvas.h"
#include <iostream>

Canvas::Canvas(int width, int height) : width(width), height(height) {
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
}

Canvas::~Canvas() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Canvas::Draw() {
    shader.Use();
    glBindVertexArray(VAO);
    glLineWidth(3.0f);

    for (const auto& stroke : strokes)
        UploadAndDraw(stroke);

    if (currentStroke.size() >= 2)
        UploadAndDraw(currentStroke);
}

void Canvas::Clear() {
    strokes.clear();
    currentStroke.clear();
}

void Canvas::OnMouseButton(int button, int action) {
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mouseDown = true;
            currentStroke.clear();
        }
        else if (action == GLFW_RELEASE) {
            mouseDown = false;
            if (!currentStroke.empty())
                strokes.push_back(currentStroke);
        }
    }
}

void Canvas::OnMouseMove(double mx, double my) {
    if (mouseDown)
        currentStroke.push_back(ToNDC(mx, my));
}

void Canvas::OnKeyPress(int key, int action) {
    if (key == GLFW_KEY_C && action == GLFW_PRESS)
        Clear();
}

Point Canvas::ToNDC(double mx, double my) {
    return {
        (float)(mx / width) * 2.0f - 1.0f,
        1.0f - (float)(my / height) * 2.0f
    };
}

void Canvas::UploadAndDraw(const std::vector<Point>& points) {
    if (points.size() < 2) return;
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, points.size() * sizeof(Point), points.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINE_STRIP, 0, (int)points.size());
}