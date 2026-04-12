#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <WS2tcpip.h>
#include "../Common/Packet.h"

class Client {
private:
    WSADATA data;
    SOCKET  serverSocket;

public:
    Client(const std::string& ipAddress, int port) {
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            throw std::runtime_error("WSAStartup failed.");

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Socket creation failed.");
        }

        sockaddr_in hint{};
        hint.sin_family = AF_INET;
        hint.sin_port   = htons(port);
        inet_pton(AF_INET, ipAddress.c_str(), &hint.sin_addr);

        if (connect(serverSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR) {
            closesocket(serverSocket);
            WSACleanup();
            throw std::runtime_error("Failed to connect to server.");
        }
    }

    bool Authenticate(const std::string& username, const std::string& password) {
        std::string credentials = username + ":" + password;
        SendString(credentials);

        Packet response = ReceivePacket();
        return response.header.type == PacketType::ACK;
    }


    void SendString(const std::string& message) {
        send(serverSocket, message.c_str(), (int)(message.size() + 1), 0);
    }

    std::string ReceiveString() {
        char buf[512];
        ZeroMemory(buf, 512);
        recv(serverSocket, buf, 512, 0);
        return std::string(buf);
    }

    void SendPacket(const Packet& packet) {
        uint32_t totalSize = 0;
        char* buf = packet.Serialize(totalSize);
        send(serverSocket, (char*)&totalSize, sizeof(uint32_t), 0);
        send(serverSocket, buf, (int)totalSize, 0);
        delete[] buf;
    }

  
    Packet ReceivePacket() {
        uint32_t totalSize = 0;
        int r = recv(serverSocket, (char*)&totalSize, sizeof(uint32_t), 0);
        if (r <= 0)
            throw std::runtime_error("ReceivePacket: connection closed or recv error on size prefix.");
        if (totalSize == 0 || totalSize > 10 * 1024 * 1024) 
            throw std::runtime_error("ReceivePacket: invalid packet size received (" + std::to_string(totalSize) + ").");

        char* buf = new char[totalSize];
        ZeroMemory(buf, totalSize);
        int received = recv(serverSocket, buf, (int)totalSize, 0);
        if (received <= 0) {
            delete[] buf;
            throw std::runtime_error("ReceivePacket: connection closed or recv error on payload.");
        }
        Packet p = Packet::Deserialize(buf, totalSize);
        delete[] buf;
        return p;
    }

    void Cleanup() {
        closesocket(serverSocket);
        WSACleanup();
    }
};
