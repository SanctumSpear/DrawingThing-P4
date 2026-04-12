#pragma once
#pragma comment(lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <vector>
#include <WS2tcpip.h>
#include "../Common/Game.h"
#include "../Common/Packet.h"

// Server: manages one listening socket and a vector of client sockets,
// one per connected player. Covers REQ-INT-010 (multiple simultaneous clients).

class Server {
private:
    WSADATA              data;
    SOCKET               serverSocket;
    std::vector<SOCKET>  clientSockets; // index 0 = first player, etc.

    // Shared send/receive helpers that work on any SOCKET
    void RawSendPacket(SOCKET sock, const Packet& packet) {
        uint32_t totalSize = 0;
        char* buf = packet.Serialize(totalSize);
        send(sock, (char*)&totalSize, sizeof(uint32_t), 0);
        send(sock, buf, (int)totalSize, 0);
        delete[] buf;
    }

    Packet RawReceivePacket(SOCKET sock) {
        uint32_t totalSize = 0;
        recv(sock, (char*)&totalSize, sizeof(uint32_t), 0);
        char* buf = new char[totalSize];
        ZeroMemory(buf, totalSize);
        recv(sock, buf, (int)totalSize, 0);
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
    // Initialises Winsock and starts listening on the given port
    Server(int port) {
        if (WSAStartup(MAKEWORD(2, 2), &data) != 0)
            throw std::runtime_error("WSAStartup failed.");

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET) {
            WSACleanup();
            throw std::runtime_error("Server socket creation failed.");
        }

        sockaddr_in hint{};
        hint.sin_family           = AF_INET;
        hint.sin_port             = htons(port);
        hint.sin_addr.S_un.S_addr = INADDR_ANY;

        bind(serverSocket, (sockaddr*)&hint, sizeof(hint));
        listen(serverSocket, SOMAXCONN);
    }

    // Blocks until one client connects; adds it to clientSockets.
    // Returns the index of the new client (use with all per-client methods).
    int AcceptClient() {
        SOCKET sock = accept(serverSocket, nullptr, nullptr);
        clientSockets.push_back(sock);
        return (int)clientSockets.size() - 1;
    }

    int GetClientCount() const { return (int)clientSockets.size(); }

    // --- Directed: send/receive to/from one specific client ---

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

    // --- Broadcast: send the same packet to every connected client ---
    void BroadcastPacket(const Packet& packet) {
        for (auto& sock : clientSockets)
            RawSendPacket(sock, packet);
    }

    void BroadcastString(const std::string& msg) {
        for (auto& sock : clientSockets)
            RawSendString(sock, msg);
    }

    // Remove a client (e.g. failed auth) and close its socket
    void CloseClient(int clientIdx) {
        if (clientIdx < 0 || clientIdx >= (int)clientSockets.size()) return;
        closesocket(clientSockets[clientIdx]);
        clientSockets.erase(clientSockets.begin() + clientIdx);
    }

    // Close everything
    void Cleanup() {
        for (auto& sock : clientSockets)
            closesocket(sock);
        clientSockets.clear();
        closesocket(serverSocket);
        WSACleanup();
    }
};
