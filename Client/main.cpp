#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>
#include "Canvas.h"
#include "../Client/client.h"
#include "../Common/Packet.h"

const int   WIDTH      = 800;
const int   HEIGHT     = 600;
const float TIME_LIMIT = 60.0f; // seconds to draw

static const std::string SERVER_IP   = "127.0.0.1";
static const int         SERVER_PORT = 54000;

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

int main(int argc, char* argv[]) {

    std::string username, password;
    if (argc >= 3) {
        username = argv[1];
        password = argv[2];
    } else {
        std::cout << "Username: ";
        std::cin >> username;
        std::cout << "Password: ";
        std::cin >> password;
    }

    Client client(SERVER_IP, SERVER_PORT);
    std::cout << "Connected to server.\n";

    bool authed = client.Authenticate(username, password);
    if (!authed) {
        std::cerr << "Authentication failed.\n";
        client.Cleanup();
        return 1;
    }
    std::cout << "Authenticated as " << username << ".\n";

    std::cout << "Waiting for game to start...\n";
    Packet startPkt = client.ReceivePacket();
    if (startPkt.header.type != PacketType::GAME_START) {
        std::cerr << "Expected GAME_START, got something else. Exiting.\n";
        client.Cleanup();
        return 1;
    }
    std::cout << "Game started!\n\n";

    Packet promptPkt = client.ReceivePacket();
    if (promptPkt.header.type != PacketType::PROMPT) {
        std::cerr << "Expected PROMPT packet. Exiting.\n";
        client.Cleanup();
        return 1;
    }

    std::string prompt    = promptPkt.GetPromptString();
    uint8_t     myId      = promptPkt.header.dstAddress; // server assigned player ID
    uint8_t     sessionId = promptPkt.header.sessionID;

    std::cout << "You are Player " << (int)myId << ".\n";
    std::cout << "Draw: \"" << prompt << "\"\n";
    std::cout << "Press ENTER or SPACE to submit. Press C to clear.\n\n";

    // ACK the prompt so the server knows we are ready to draw
    Packet ack = Packet::MakeAckPacket(sessionId, myId, 0);
    client.SendPacket(ack);

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

    std::vector<unsigned char> exportedPixels;
    bool pixelsReady = false;

    auto onSubmit = [&](std::vector<unsigned char> pixels) {
        exportedPixels = std::move(pixels);
        pixelsReady    = true;
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

    // Force-submit if the window was closed without pressing ENTER/SPACE
    if (!canvas.IsDone())
        canvas.OnKeyPress(GLFW_KEY_ENTER, GLFW_PRESS);

    gCanvas = nullptr;
    glfwDestroyWindow(window);
    glfwTerminate();

    if (pixelsReady) {
        std::cout << "Sending drawing to server ("
                  << exportedPixels.size() << " bytes)...\n";
        Packet imgPkt = Packet::MakeImagePacket(
            sessionId,
            (const char*)exportedPixels.data(),
            (uint32_t)exportedPixels.size(),
            myId, 0);
        client.SendPacket(imgPkt);
        std::cout << "Drawing sent.\n\n";
    }

    std::cout << "Waiting for all drawings and vote request...\n";

    bool voted = false;
    while (!voted) {
        Packet pkt = client.ReceivePacket();

        if (pkt.header.type == PacketType::IMAGE) {
            // A broadcast drawing from another (or this) player
            std::cout << "  Received drawing from Player "
                      << (int)pkt.header.srcAddress
                      << " (" << pkt.header.payloadSize << " bytes).\n";
        }
        else if (pkt.header.type == PacketType::VOTE_REQUEST) {
            // Parse the "ID:Name\nID:Name" player list
            std::string playerList = pkt.GetPromptString();
            std::vector<std::pair<int, std::string>> options;

            std::istringstream ss(playerList);
            std::string line;
            while (std::getline(ss, line)) {
                auto pos = line.find(':');
                if (pos == std::string::npos) continue;
                int         id   = std::stoi(line.substr(0, pos));
                std::string name = line.substr(pos + 1);
                options.push_back({id, name});
            }

            std::cout << "\n=== VOTE FOR THE BEST DRAWING ===\n";
            std::cout << "You are Player " << (int)myId << ".\n";
            std::cout << "Vote for one of the following players:\n";
            for (const auto& opt : options) {
                if (opt.first == (int)myId) continue; // can't vote for yourself
                std::cout << "  [" << opt.first << "] " << opt.second << "\n";
            }

            int choice = 0;
            bool validChoice = false;
            while (!validChoice) {
                std::cout << "Enter player number: ";
                if (!(std::cin >> choice)) {
                    std::cin.clear();
                    std::cin.ignore(1024, '\n');
                    std::cout << "Invalid input. Try again.\n";
                    continue;
                }
                if (choice == (int)myId) {
                    std::cout << "You cannot vote for yourself!\n";
                    continue;
                }
                // Check the choice is in the list
                bool found = false;
                for (const auto& opt : options)
                    if (opt.first == choice) { found = true; break; }
                if (!found) {
                    std::cout << "Invalid player number. Try again.\n";
                    continue;
                }
                validChoice = true;
            }

            Packet votePkt = Packet::MakeVotePacket(
                sessionId, (uint8_t)choice, myId, 0);
            client.SendPacket(votePkt);
            std::cout << "Vote cast for Player " << choice << ".\n\n";
            voted = true;
        }
        else {
            // Unexpected packet during voting — log and break to avoid hanging
            std::cerr << "Unexpected packet type 0x"
                      << std::hex << (int)pkt.header.type
                      << std::dec << " during voting.\n";
            break;
        }
    }

    Packet resultsPkt = client.ReceivePacket();
    if (resultsPkt.header.type == PacketType::RESULTS) {
        std::cout << "\n" << resultsPkt.GetPromptString() << "\n";
    }

    Packet endPkt = client.ReceivePacket();
    if (endPkt.header.type == PacketType::GAME_END)
        std::cout << "Game over. Session "
                  << (int)endPkt.header.sessionID << " ended.\n";

    client.Cleanup();
    return 0;
}
