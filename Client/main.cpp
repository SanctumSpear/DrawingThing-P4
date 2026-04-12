#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <string>
#include "Canvas.h"
#include "../Client/client.h"
#include "../Common/Packet.h"

const int   WIDTH = 800;
const int   HEIGHT = 600;
const float TIME_LIMIT = 60.0f; // seconds to draw

static const std::string SERVER_IP = "127.0.0.1";
static const int         SERVER_PORT = 54000;
static const std::string USERNAME = "Alice";
static const std::string PASSWORD = "password123";

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
    // connect/auth
    Client client(SERVER_IP, SERVER_PORT);
    std::cout << "Connected to server.\n";

    bool authed = client.Authenticate(USERNAME, PASSWORD);
    if (!authed) {
        std::cerr << "Authentication failed.\n";
        client.Cleanup();
        return 1;
    }
    std::cout << "Authenticated as " << USERNAME << ".\n";

    //wait for game start
    std::cout << "Waiting for game start...\n";
    Packet startPkt = client.ReceivePacket();
    if (startPkt.header.type != PacketType::GAME_START) {
        std::cerr << "Expected GAME_START, got something else.\n";
        client.Cleanup();
        return 1;
    }
    std::cout << "Game started!\n";

    //check if we are drawing
    Packet next = client.ReceivePacket();
    if (next.header.type != PacketType::PROMPT) {
        // Not the drawer — just wait for game end
        std::cout << "Not the drawer this round. Waiting...\n";
        Packet endPkt = client.ReceivePacket();
        if (endPkt.header.type == PacketType::GAME_END)
            std::cout << "Game over.\n";
        client.Cleanup();
        return 0;
    }

   
    std::string prompt = next.GetPromptString();
    std::cout << "You are the drawer!\n";
    std::cout << "Draw: \"" << prompt << "\"\n";
    std::cout << "Press ENTER or SPACE to submit. Press C to clear.\n\n";

    // ACK to server
    Packet ack = Packet::MakeAckPacket(
        next.header.sessionID, next.header.dstAddress, 0);
    client.SendPacket(ack);

    //setup window
    if (!glfwInit()) return -1;

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    std::string title = "Draw: " + prompt + "  |  ENTER to submit, C to clear";
    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, title.c_str(), nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "GLAD init failed\n";
        return -1;
    }

    //drawing loop
    std::vector<unsigned char> exportedPixels;
    bool pixelsReady = false;

    auto onSubmit = [&](std::vector<unsigned char> pixels) {
        exportedPixels = std::move(pixels);
        pixelsReady = true;
        };

    Canvas canvas(WIDTH, HEIGHT, TIME_LIMIT, onSubmit);
    gCanvas = &canvas;

    glfwSetMouseButtonCallback(window, mouseButtonCallback);
    glfwSetCursorPosCallback(window, cursorPosCallback);
    glfwSetKeyCallback(window, keyCallback);

    // loop until canvas is submitted
    while (!glfwWindowShouldClose(window) && !canvas.IsDone()) {
        canvas.Update();

        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        canvas.Draw();

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // if window closed without submitting then submit
    if (!canvas.IsDone())
        canvas.OnKeyPress(GLFW_KEY_ENTER, GLFW_PRESS);

    gCanvas = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();

    // send drawing to server
    if (pixelsReady) {
        std::cout << "Sending image to server (" << exportedPixels.size() << " bytes)...\n";
        Packet imgPkt = Packet::MakeImagePacket(
            next.header.sessionID,
            (const char*)exportedPixels.data(),
            (uint32_t)exportedPixels.size(),
            next.header.dstAddress, 0);
        client.SendPacket(imgPkt);
        std::cout << "Image sent.\n";
    }

    // wait for game end
    Packet endPkt = client.ReceivePacket();
    if (endPkt.header.type == PacketType::GAME_END)
        std::cout << "Game over. Session " << (int)endPkt.header.sessionID << " ended.\n";

    client.Cleanup();
    return 0;
}