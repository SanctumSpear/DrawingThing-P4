#include "Canvas.h"
#include <iostream>

Canvas::Canvas(int width, int height, float timeLimitSeconds, std::function<void(std::vector<unsigned char>)> onSubmit)
    : width(width), height(height), timeLimit(timeLimitSeconds),
    timeRemaining(timeLimitSeconds), onSubmit(onSubmit), done(false) {

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    lastFrameTime = glfwGetTime();
}

Canvas::~Canvas() {
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
}

void Canvas::Update() {
    if (done) return;

    double now = glfwGetTime();
    float delta = (float)(now - lastFrameTime);
    lastFrameTime = now;

    timeRemaining -= delta;

    if (timeRemaining <= 0.0f) {
        timeRemaining = 0.0f;
        std::cout << "Time is up! Submitting drawing...\n";
        Submit();
    }
}

void Canvas::Draw() {
    shader.Use();
    glBindVertexArray(VAO);
    glLineWidth(3.0f);

    for (const auto& stroke : strokes)
        UploadAndDraw(stroke);

    if (currentStroke.size() >= 2)
        UploadAndDraw(currentStroke);

    DrawTimerBar();
}

// Draws a colored bar at the top of the screen showing time remaining
// Green -> Yellow -> Red as time runs out
void Canvas::DrawTimerBar() {
    float ratio = timeRemaining / timeLimit;

    // Bar goes from -1 to (ratio * 2 - 1) across the top
    float barRight = ratio * 2.0f - 1.0f;
    float barTop = 0.95f;
    float barBot = 0.88f;

    // Color: green when full, red when empty
    // We do this by drawing with a simple colored quad
    // For simplicity we use the same shader but change clear color trick
    // A proper solution would pass color as uniform — fine to upgrade later
    std::vector<Point> bar = {
        { -1.0f, barTop },
        { barRight, barTop },
        { barRight, barBot },
        { -1.0f, barBot }
    };

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, bar.size() * sizeof(Point), bar.data(), GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(Point), (void*)0);
    glEnableVertexAttribArray(0);
    glDrawArrays(GL_LINE_LOOP, 0, (int)bar.size());
}

bool Canvas::IsDone() {
    return done;
}

void Canvas::Clear() {
    strokes.clear();
    currentStroke.clear();
}

void Canvas::Submit() {
    if (done) return;
    done = true;

    // Finish any stroke in progress
    if (!currentStroke.empty())
        strokes.push_back(currentStroke);

    std::cout << "Exporting drawing...\n";
    std::vector<unsigned char> pixels = ExportPixels();
    std::cout << "Drawing exported (" << pixels.size() << " bytes), sending...\n";

    onSubmit(pixels);
}

std::vector<unsigned char> Canvas::ExportPixels() {
    std::vector<unsigned char> pixels(width * height * 3);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    return pixels;
}

void Canvas::OnMouseButton(int button, int action) {
    if (done) return;
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
    if (done) return;
    if (mouseDown)
        currentStroke.push_back(ToNDC(mx, my));
}

void Canvas::OnKeyPress(int key, int action) {
    if (action != GLFW_PRESS) return;
    if (key == GLFW_KEY_C)
        Clear();
    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) {
        std::cout << "Player submitted drawing.\n";
        Submit();
    }
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