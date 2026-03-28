#pragma once
#pragma comment (lib, "ws2_32.lib")
#include <iostream>
#include <string>
#include <WS2tcpip.h>

class Server {
private:
	WSADATA data;
	SOCKET serverSocket;
	SOCKET clientSocket;

public:

	//Initializes Winsock and creates server socket
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

	//Accepts a client connection
    void AcceptClient() {
        clientSocket = accept(serverSocket, nullptr, nullptr);
    }

    //Sends a string to client
    void SendString(const std::string& message) {
        send(clientSocket, message.c_str(), message.size() + 1, 0);
    }

    //Receive a string from client
    std::string ReceiveString() {
        char buffer[512];
        ZeroMemory(buffer, 512);
        recv(clientSocket, buffer, 512, 0);
        return std::string(buffer);
    }

    //Closes server, sockets, and cleans up Winsock
    void Cleanup() {
        closesocket(clientSocket);
        closesocket(serverSocket);
        WSACleanup();
    }
};