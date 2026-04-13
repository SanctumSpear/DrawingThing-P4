#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include "../Common/Game.h"
#include "../Common/Packet.h"

class Server {
private:
    WSADATA              data;
    SOCKET               serverSocket;
    std::vector<SOCKET>  clientSockets;

    static bool RecvAll(SOCKET sock, char* buf, int len) {
        int total = 0;
        while (total < len) {
            int r = recv(sock, buf + total, len - total, 0);
            if (r <= 0) return false;
            total += r;
        }
        return true;
    }

    static bool SendAll(SOCKET sock, const char* buf, int len) {
        int total = 0;
        while (total < len) {
            int sent = send(sock, buf + total, len - total, 0);
            if (sent == SOCKET_ERROR) return false;
            total += sent;
        }
        return true;
    }

    void RawSendPacket(SOCKET sock, const Packet& packet) {
        uint32_t totalSize = 0;
        char* buf = packet.Serialize(totalSize);

        // Send size prefix
        if (!SendAll(sock, (char*)&totalSize, sizeof(uint32_t))) {
            delete[] buf;
            throw std::runtime_error("RawSendPacket: failed sending size prefix.");
        }

        // Send full payload in chunks
        if (!SendAll(sock, buf, (int)totalSize)) {
            delete[] buf;
            throw std::runtime_error("RawSendPacket: failed sending payload.");
        }

        delete[] buf;
    }

    Packet RawReceivePacket(SOCKET sock) {
        uint32_t totalSize = 0;
        if (!RecvAll(sock, (char*)&totalSize, sizeof(uint32_t)))
            throw std::runtime_error("RawReceivePacket: connection closed reading size prefix.");

        if (totalSize < sizeof(PacketHeader) || totalSize > 10 * 1024 * 1024)
            throw std::runtime_error("RawReceivePacket: invalid packet size (" + std::to_string(totalSize) + ").");

        char* buf = new char[totalSize];
        ZeroMemory(buf, totalSize);
        if (!RecvAll(sock, buf, (int)totalSize)) {
            delete[] buf;
            throw std::runtime_error("RawReceivePacket: connection closed reading payload.");
        }

        Packet p = Packet::Deserialize(buf, totalSize);
        delete[] buf;
        return p;
    }

    void RawSendString(SOCKET sock, const std::string& msg) {
        send(sock, msg.c_str(), (int)(msg.size() + 1), 0);
    }

    std::string RawReceiveString(SOCKET sock) {
        char buf[512];
        ZeroMemory(buf, 512);
        recv(sock, buf, 512, 0);
        return std::string(buf);
    }

public:
    Server(int port) {
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            throw std::runtime_error("WSAStartup failed.");

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Server socket creation failed.");
        }

        sockaddr_in hint{};
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        hint.sin_addr.S_un.S_addr = INADDR_ANY;

        bind(serverSocket, (sockaddr*)&hint, sizeof(hint));
        listen(serverSocket, SOMAXCONN);
    }

    int AcceptClient() {
        SOCKET sock = accept(serverSocket, nullptr, nullptr);
        clientSockets.push_back(sock);
        return (int)clientSockets.size() - 1;
    }

    int GetClientCount() const { return (int)clientSockets.size(); }

    void SendPacket(int clientIdx, const Packet& packet) {
        RawSendPacket(clientSockets.at(clientIdx), packet);
    }

    Packet ReceivePacket(int clientIdx) {
        return RawReceivePacket(clientSockets.at(clientIdx));
    }

    void SendString(int clientIdx, const std::string& msg) {
        RawSendString(clientSockets.at(clientIdx), msg);
    }

    std::string ReceiveString(int clientIdx) {
        return RawReceiveString(clientSockets.at(clientIdx));
    }

    void BroadcastPacket(const Packet& packet) {
        for (auto& sock : clientSockets)
            RawSendPacket(sock, packet);
    }

    void BroadcastString(const std::string& msg) {
        for (auto& sock : clientSockets)
            RawSendString(sock, msg);
    }

    void CloseClient(int clientIdx) {
        if (clientIdx < 0 || clientIdx >= (int)clientSockets.size()) return;
        closesocket(clientSockets[clientIdx]);
        clientSockets.erase(clientSockets.begin() + clientIdx);
    }

    void Cleanup() {
        for (auto& sock : clientSockets)
            closesocket(sock);
        clientSockets.clear();
        closesocket(serverSocket);
        WSACleanup();
    }
};