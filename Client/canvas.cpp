#include "Canvas.h"
#include <iostream>

Canvas::Canvas(int width, int height, float timeLimitSeconds,
    std::function<void(std::vector<unsigned char>)> onSubmit)
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
    float  delta = (float)(now - lastFrameTime);
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

    // Draw all completed strokes using their locked-in color
    for (const auto& stroke : strokes) {
        shader.SetColor(stroke.r, stroke.g, stroke.b);
        UploadAndDraw(stroke.points);
    }

    // Draw current in-progress stroke using color locked at press time
    if (currentStroke.size() >= 2) {
        shader.SetColor(strokeR, strokeG, strokeB);
        UploadAndDraw(currentStroke);
    }

    // Timer bar always black
    shader.SetColor(0.0f, 0.0f, 0.0f);
    DrawTimerBar();

    // Reset shader back to active color for next frame
    shader.SetColor(r, g, b);
}

void Canvas::DrawTimerBar() {
    float ratio = timeRemaining / timeLimit;
    float barRight = ratio * 2.0f - 1.0f;
    float barTop = 0.95f;
    float barBot = 0.88f;

    std::vector<Point> bar = {
        { -1.0f,    barTop },
        { barRight, barTop },
        { barRight, barBot },
        { -1.0f,    barBot }
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

    if (!currentStroke.empty())
        strokes.push_back({ currentStroke, strokeR, strokeG, strokeB });

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

void Canvas::SetColor(float r, float g, float b) {
    this->r = r;
    this->g = g;
    this->b = b;
}

void Canvas::OnMouseButton(int button, int action) {
    if (done) return;
    if (button == GLFW_MOUSE_BUTTON_LEFT) {
        if (action == GLFW_PRESS) {
            mouseDown = true;
            currentStroke.clear();
            // Lock color at the moment drawing starts
            strokeR = r;
            strokeG = g;
            strokeB = b;
        }
        else if (action == GLFW_RELEASE) {
            mouseDown = false;
            if (!currentStroke.empty())
                strokes.push_back({ currentStroke, strokeR, strokeG, strokeB });
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

    if (key == GLFW_KEY_C)                              Clear();
    if (key == GLFW_KEY_ENTER || key == GLFW_KEY_SPACE) Submit();

    if (key == GLFW_KEY_1) SetColor(0.0f, 0.0f, 0.0f); // black
    if (key == GLFW_KEY_2) SetColor(1.0f, 0.0f, 0.0f); // red
    if (key == GLFW_KEY_3) SetColor(0.0f, 0.8f, 0.0f); // green
    if (key == GLFW_KEY_4) SetColor(0.0f, 0.4f, 1.0f); // blue
    if (key == GLFW_KEY_5) SetColor(1.0f, 0.6f, 0.0f); // orange
    if (key == GLFW_KEY_6) SetColor(0.6f, 0.0f, 0.8f); // purple
    if (key == GLFW_KEY_7) SetColor(1.0f, 1.0f, 0.0f); // yellow
    if (key == GLFW_KEY_8) SetColor(1.0f, 1.0f, 1.0f); // white eraser
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