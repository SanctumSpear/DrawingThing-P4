#pragma once
#pragma comment (lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include "../Common/Packet.h"

class Client {
private:
    WSADATA data;
    SOCKET serverSocket;

public:

    Client(const std::string& ipAddress, int port) {
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0) {
            throw std::runtime_error("WSAStartup failed.");
        }

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Socket creation failed.");
        }

        sockaddr_in hint;
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

        if (connect(serverSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Failed to connect to server.");
        }
    }

    // Sends a string to the server
    void SendString(const std::string& message) {
        send(serverSocket, message.c_str(), message.size() + 1, 0);
    }

    // Receives a string from the server
    std::string ReceiveString() {
        char buffer[512];
        ZeroMemory(buffer, 512);
        recv(serverSocket, buffer, 512, 0);
        return std::string(buffer);
    }

    // Serializes a Packet and sends it to the server.
    // Sends total byte size first (4 bytes) so the server
    // knows how much to read, then sends the raw packet bytes.
    void SendPacket(const Packet& packet) {
        uint32_t totalSize = 0;
        char* buffer = packet.Serialize(totalSize);

        send(serverSocket, (char*)&totalSize, sizeof(uint32_t), 0);
        send(serverSocket, buffer, (int)totalSize, 0);

        delete[] buffer;
    }

    // Reads the size prefix then the packet bytes and
    // deserializes them back into a Packet object.
    Packet ReceivePacket() {
        uint32_t totalSize = 0;
        recv(serverSocket, (char*)&totalSize, sizeof(uint32_t), 0);

        char* buffer = new char[totalSize];
        ZeroMemory(buffer, totalSize);
        recv(serverSocket, buffer, (int)totalSize, 0);

        Packet packet = Packet::Deserialize(buffer, totalSize);
        delete[] buffer;
        return packet;
    }

    // Closes the socket and cleans up Winsock
    void Cleanup() {
        closesocket(serverSocket);
        WSACleanup();
    }
};