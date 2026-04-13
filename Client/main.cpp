#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <QApplication>
#include "ClientWindow.h"
#include "Canvas.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>

const int   WIDTH = 800;
const int   HEIGHT = 600;
const float TIME_LIMIT = 60.0f;

Canvas* gCanvas = nullptr;

void mouseButtonCallback(GLFWwindow* w, int button, int action, int mods) {
    if (gCanvas) gCanvas->OnMouseButton(button, action);
}
void cursorPosCallback(GLFWwindow* w, double mx, double my) {
    if (gCanvas) gCanvas->OnMouseMove(mx, my);
}
void keyCallback(GLFWwindow* w, int key, int scan, int action, int mods) {
    if (gCanvas) gCanvas->OnKeyPress(key, action);
}

std::vector<unsigned char> runCanvas(const std::string& prompt) {
    if (!glfwInit()) return {};

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string title = "Draw: " + prompt +
        "  |  1-8: colors  |  8: eraser  |  C: clear  |  ENTER: submit";
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), nullptr, nullptr);
    if (!window) { glfwTerminate(); return {}; }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        glfwTerminate();
        return {};
    }

    std::vector<unsigned char> exportedPixels;

    auto onSubmit = [&](std::vector<unsigned char> pixels) {
        exportedPixels = std::move(pixels);
        };

    Canvas canvas(WIDTH, HEIGHT, TIME_LIMIT, onSubmit);
    gCanvas = &canvas;

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    while (!glfwWindowShouldClose(window) && !canvas.IsDone()) {
        canvas.Update();
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        canvas.Draw();
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    if (!canvas.IsDone())
        canvas.OnKeyPress(GLFW_KEY_ENTER, GLFW_PRESS);

    gCanvas = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();

    return exportedPixels;
}

int main(int argc, char* argv[]) {
    QApplication app(argc, argv);

    ClientWindow window;
    window.show();

    // When prompt arrives, open canvas then send pixels to worker
    QObject::connect(window.worker, &ClientWorker::promptReceived,
        [&](QString prompt, int sessionID, int myPlayerId) {
            std::vector<unsigned char> pixels = runCanvas(prompt.toStdString());

            if (!pixels.empty()) {
                QMetaObject::invokeMethod(window.worker, "sendImage",
                    Qt::QueuedConnection,
                    Q_ARG(QByteArray, QByteArray(
                        (const char*)pixels.data(), (int)pixels.size())),
                    Q_ARG(int, sessionID),
                    Q_ARG(int, myPlayerId));
            }
        });

    return app.exec();
}