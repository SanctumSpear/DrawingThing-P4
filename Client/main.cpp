#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include "client.h"

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

int main() {
    if (!glfwInit()) {
        std::cerr << "GLFW init failed\n";
        return -1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(800, 600, "Drawing Client", nullptr, nullptr);
    if (!window) {
        std::cerr << "Window creation failed\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n";
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << "\n";

    while (!glfwWindowShouldClose(window)) {
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}