#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ClientWorker.h"
#include "../Common/Packet.h"
#include <QCoreApplication>
#include <iostream>
#include <stdexcept>

ClientWorker::ClientWorker(QObject* parent) : QObject(parent) {}

void ClientWorker::connectAndRun(QString ip, int port, QString username, QString password) {
    try {
        if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
            throw std::runtime_error("WSAStartup failed.");

        serverSocket = socket(AF_INET, SOCK_STREAM, 0);
        if (serverSocket == INVALID_SOCKET)
            throw std::runtime_error("Socket creation failed.");

        sockaddr_in hint{};
        hint.sin_family = AF_INET;
        hint.sin_port = htons(port);
        inet_pton(AF_INET, ip.toStdString().c_str(), &hint.sin_addr);

        if (::connect(serverSocket, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
            throw std::runtime_error("Failed to connect to server.");

        emit logMessage("Connected to server.");

        // Authenticate
        std::string creds = username.toStdString() + ":" + password.toStdString();
        sendString(creds);

        Packet authResp = receivePacket();
        if (authResp.header.type != PacketType::ACK) {
            emit loginFailed("Authentication failed. Check credentials.");
            closesocket(serverSocket);
            WSACleanup();
            return;
        }

        int myId = (int)authResp.header.dstAddress;
        currentSessionID = (int)authResp.header.sessionID;
        emit loginSuccess(username);
        emit logMessage("Authenticated as " + username);

        // Wait for game start
        emit logMessage("Waiting for game to start...");
        Packet startPkt = receivePacket();
        if (startPkt.header.type != PacketType::GAME_START) {
            emit errorOccurred("Expected GAME_START.");
            return;
        }
        emit gameStarted();

        // Receive prompt
        Packet promptPkt = receivePacket();
        if (promptPkt.header.type != PacketType::PROMPT) {
            emit errorOccurred("Expected PROMPT packet.");
            return;
        }

        std::string prompt = promptPkt.GetPromptString();
        currentSessionID = (int)promptPkt.header.sessionID;

        // Send ACK before emitting so server doesn't time out
        Packet ack = Packet::MakeAckPacket(
            promptPkt.header.sessionID,
            promptPkt.header.dstAddress, 0);
        sendPacket(ack);
        emit logMessage("ACK sent.");

        // Signal main thread to open the canvas
        emit promptReceived(QString::fromStdString(prompt), currentSessionID, myId);
        emit logMessage("Waiting for drawing to be submitted...");

        // Wait for sendImage to be called from main thread
        while (!imageReady) {
            QThread::msleep(50);
            QCoreApplication::processEvents();
        }
        emit logMessage("Drawing submitted, waiting for other players...");

        // Receive all drawings broadcasted by server
        emit logMessage("Waiting for drawings from server...");
        while (true) {
            Packet pkt = receivePacket();
            std::cout << "Received packet type: " << (int)pkt.header.type << "\n";
            std::cout << "From player: " << (int)pkt.header.srcAddress << "\n";

            if (pkt.header.type == PacketType::IMAGE) {
                int drawerId = (int)pkt.header.srcAddress;
                std::string drawerName = "Player " + std::to_string(drawerId);
                QByteArray pixels((const char*)pkt.data, (int)pkt.header.payloadSize);
                emit drawingReceived(drawerId, QString::fromStdString(drawerName), pixels, 800, 600);
                emit logMessage(QString("Drawing received from player %1.").arg(drawerId));
            }
            else if (pkt.header.type == PacketType::VOTE_REQUEST) {
                std::string playerList = pkt.GetPromptString();
                emit voteRequestReceived(QString::fromStdString(playerList));
                emit logMessage("Vote request received.");
                break;
            }
            else {
                // This is the problem — something unexpected is breaking the loop
                std::cout << "Unexpected packet type: " << (int)pkt.header.type
                    << " breaking out of loop early!\n";
                break;
            }
        }

        // Wait for vote from UI
        emit logMessage("Waiting for your vote...");
        while (!voteReady) {
            QThread::msleep(100);
            QCoreApplication::processEvents();
        }

        Packet votePkt = Packet::MakeVotePacket(
            (uint8_t)currentSessionID,
            (uint8_t)pendingVote, 0);
        sendPacket(votePkt);
        emit logMessage(QString("Vote sent for player %1.").arg(pendingVote));

        // Receive results
        Packet resultsPkt = receivePacket();
        if (resultsPkt.header.type == PacketType::RESULTS) {
            std::string results = resultsPkt.GetPromptString();
            emit resultsReceived(QString::fromStdString(results));
        }

        // Receive game end
        Packet endPkt = receivePacket();
        if (endPkt.header.type == PacketType::GAME_END) {
            emit gameEnded();
            emit logMessage("Game over.");
        }

        closesocket(serverSocket);
        WSACleanup();

    }
    catch (const std::exception& e) {
        emit errorOccurred(QString("Error: %1").arg(e.what()));
        if (serverSocket != INVALID_SOCKET) {
            closesocket(serverSocket);
            WSACleanup();
        }
    }
}

void ClientWorker::registerAccount(QString ip, int port,
    QString username, QString password)
{
    // Use a dedicated short-lived socket so this doesn't touch serverSocket,
    // which is reserved for the full game session in connectAndRun().
    WSADATA regWsa;
    SOCKET  regSock = INVALID_SOCKET;

    try {
        if (WSAStartup(MAKEWORD(2, 2), &regWsa) != 0)
            throw std::runtime_error("WSAStartup failed.");

        regSock = socket(AF_INET, SOCK_STREAM, 0);
        if (regSock == INVALID_SOCKET)
            throw std::runtime_error("Socket creation failed.");

        sockaddr_in hint{};
        hint.sin_family = AF_INET;
        hint.sin_port   = htons(port);
        inet_pton(AF_INET, ip.toStdString().c_str(), &hint.sin_addr);

        if (::connect(regSock, (sockaddr*)&hint, sizeof(hint)) == SOCKET_ERROR)
            throw std::runtime_error("Failed to connect to server.");

        // Send "REGISTER:username:password" — the server strips the prefix and
        // calls AccountManager::CreateAccount() before closing the connection.
        std::string creds = "REGISTER:" + username.toStdString()
                          + ":" + password.toStdString();
        send(regSock, creds.c_str(), (int)(creds.size() + 1), 0);

        // Receive the length-prefixed response packet
        uint32_t totalSize = 0;
        recv(regSock, (char*)&totalSize, sizeof(uint32_t), 0);
        char* buf = new char[totalSize];
        int received = 0;
        while (received < (int)totalSize) {
            int r = recv(regSock, buf + received, (int)totalSize - received, 0);
            if (r <= 0) { delete[] buf; throw std::runtime_error("Connection closed during registration."); }
            received += r;
        }
        Packet resp = Packet::Deserialize(buf, totalSize);
        delete[] buf;

        closesocket(regSock);
        WSACleanup();

        if (resp.header.type == PacketType::ACK)
            emit registerSuccess(username);
        else
            emit registerFailed("Registration failed: username may already be taken.");
    }
    catch (const std::exception& e) {
        if (regSock != INVALID_SOCKET) closesocket(regSock);
        WSACleanup();
        emit registerFailed(QString("Error: %1").arg(e.what()));
    }
}

void ClientWorker::sendVote(int votedPlayerId) {
    pendingVote = votedPlayerId;
    voteReady = true;
}

void ClientWorker::sendImage(QByteArray pixels, int sessionID, int myPlayerId) {
    Packet imgPkt = Packet::MakeImagePacket(
        (uint8_t)sessionID,
        pixels.constData(),
        (uint32_t)pixels.size(),
        0, (uint8_t)myPlayerId);
    sendPacket(imgPkt);
    imageReady = true;
    emit logMessage(QString("Image sent (%1 bytes).").arg(pixels.size()));
}

void ClientWorker::sendPacket(const Packet& pkt) {
    uint32_t totalSize = 0;
    char* buf = pkt.Serialize(totalSize);
    send(serverSocket, (char*)&totalSize, sizeof(uint32_t), 0);
    send(serverSocket, buf, (int)totalSize, 0);
    delete[] buf;
}

Packet ClientWorker::receivePacket() {
    uint32_t totalSize = 0;
    recv(serverSocket, (char*)&totalSize, sizeof(uint32_t), 0);

    char* buf = new char[totalSize];
    int totalReceived = 0;
    while (totalReceived < (int)totalSize) {
        int r = recv(serverSocket,
            buf + totalReceived,
            (int)totalSize - totalReceived, 0);
        if (r <= 0) {
            delete[] buf;
            throw std::runtime_error("receivePacket failed mid-receive.");
        }
        totalReceived += r;
    }

    Packet p = Packet::Deserialize(buf, totalSize);
    delete[] buf;
    return p;
}

void ClientWorker::sendString(const std::string& str) {
    send(serverSocket, str.c_str(), (int)(str.size() + 1), 0);
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      