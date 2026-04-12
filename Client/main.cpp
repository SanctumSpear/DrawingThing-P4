#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include "canvas.h"
#include "client.h"
#include <iostream>



//int main() {
//    try {
//        Client client("127.0.0.1", 27500);
//        std::cout << "Connected to server!\n";
//
//        // Send a message to the server
//        client.SendString("Hello from client!");
//        std::cout << "Client: Hello from client!\n";
//
//        // Receive the server's response
//        std::string received = client.ReceiveString();
//        std::cout << "Server: " << received << "\n";
//
//        client.Cleanup();
//    }
//    catch (const std::exception& e) {
//        std::cerr << "Client error: " << e.what() << "\n";
//        return 1;
//    }
//
//    return 0;
//}

const int WIDTH = 800;
const int HEIGHT = 600;

// Global pointer so static callbacks can reach the Canvas
Canvas* gCanvas = nullptr;

void mouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
    if (gCanvas) gCanvas->OnMouseButton(button, action);
}

void cursorPosCallback(GLFWwindow* window, double mx, double my) {
    if (gCanvas) gCanvas->OnMouseMove(mx, my);
}

void keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    if (gCanvas) gCanvas->OnKeyPress(key, action);
}

int main() {
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Drawing Client", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n";
        return -1;
    }

    Canvas canvas(WIDTH, HEIGHT);
    gCanvas = &canvas;

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        canvas.Draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    gCanvas = nullptr;
    glfwTerminate();
    return 0;
}