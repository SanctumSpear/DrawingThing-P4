#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <functional>
#include "Shader.h"

struct Point { float x, y; };

class Canvas {
public:
    // onSubmit is called when user submits or timer runs out
    // passes the raw pixel data back to whoever created the Canvas
    Canvas(int width, int height, float timeLimitSeconds, std::function<void(std::vector<unsigned char>)> onSubmit);
    ~Canvas();

    void Draw();
    void Clear();
    void Update(); // call every frame — handles timer

    bool IsDone(); // returns true when drawing is submitted

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
    bool done = false;

    float timeLimit;
    float timeRemaining;
    double lastFrameTime;

    std::function<void(std::vector<unsigned char>)> onSubmit;

    void Submit();
    std::vector<unsigned char> ExportPixels();
    Point ToNDC(double mx, double my);
    void UploadAndDraw(const std::vector<Point>& points);
    void DrawTimerBar();
};