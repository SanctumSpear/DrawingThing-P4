#pragma once
#pragma comment (lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include "../Common/Game.h"
#include "../Common/Packet.h"

class Server {
private:
    WSADATA data;
    SOCKET  serverSocket;
    SOCKET  clientSocket;

public:

    // Initializes Winsock and creates server socket
    Server(int port) {
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed.");
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Server socket creation failed.");
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        hint.sin_addr.S_un.S_addr = INADDR_ANY;

        bind(serverSocket, (sockaddr*)&hint, sizeof(hint));
        listen(serverSocket, SOMAXCONN);
    }

    // Accepts a client connection
    void AcceptClient() {
        clientSocket = accept(serverSocket, nullptr, nullptr);
    }

    // Sends a raw string to the client
    void SendString(const std::string& message) {
        send(clientSocket, message.c_str(), (int)(message.size() + 1), 0);
    }

    // Receives a raw string from the client
    std::string ReceiveString() {
        char buffer[512];
        ZeroMemory(buffer, 512);
        recv(clientSocket, buffer, 512, 0);
        return std::string(buffer);
    }

    // Serializes a Packet and sends it over the socket.
    // Sends the total byte size first (4 bytes) so the receiver
    // knows how much to read, then sends the raw packet bytes.
    void SendPacket(const Packet& packet) {
        uint32_t totalSize = 0;
        char* buffer = packet.Serialize(totalSize);

        // Send size header so receiver can allocate the right buffer
        send(clientSocket, (char*)&totalSize, sizeof(uint32_t), 0);
        send(clientSocket, buffer, (int)totalSize, 0);

        delete[] buffer;
    }

    // Reads the size prefix, then the packet bytes, and deserializes
    // them back into a Packet object.
    Packet ReceivePacket() {
        uint32_t totalSize = 0;
        recv(clientSocket, (char*)&totalSize, sizeof(uint32_t), 0);

        char* buffer = new char[totalSize];
        ZeroMemory(buffer, totalSize);
        recv(clientSocket, buffer, (int)totalSize, 0);

        Packet packet = Packet::Deserialize(buffer, totalSize);
        delete[] buffer;
        return packet;
    }

    // Closes both sockets and shuts down Winsock
    void Cleanup() {
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
    }
};
