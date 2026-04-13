#pragma once
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <vector>
#include <functional>
#include "Shader.h"

struct Point { float x, y; };

class Canvas {
public:
    Canvas(int width, int height, float timeLimitSeconds, std::function<void(std::vector<unsigned char>)> onSubmit);
    ~Canvas();

    void Draw();
    void Clear();
    void Update();
    void SetColor(float r, float g, float b);
    bool IsDone();

    void OnMouseButton(int button, int action);
    void OnMouseMove(double mx, double my);
    void OnKeyPress(int key, int action);

private:
    int width, height;
    unsigned int VAO, VBO;
    Shader shader;

    struct Stroke {
        std::vector<Point> points;
        float r, g, b;
    };

    std::vector<Stroke> strokes;
    std::vector<Point>  currentStroke;
    bool mouseDown = false;
    bool done = false;

    // Active color (changes when user picks a color)
    float r = 0.0f, g = 0.0f, b = 0.0f;

    // Color locked in at the moment mouse is pressed
    float strokeR = 0.0f, strokeG = 0.0f, strokeB = 0.0f;

    float  timeLimit;
    float  timeRemaining;
    double lastFrameTime;

    std::function<void(std::vector<unsigned char>)> onSubmit;

    void Submit();
    std::vector<unsigned char> ExportPixels();
    Point ToNDC(double mx, double my);
    void UploadAndDraw(const std::vector<Point>& points);
    void DrawTimerBar();
};