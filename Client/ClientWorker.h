#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include <QObject>
#include <QByteArray>
#include <QString>
#include <QThread>
#include <QCoreApplication>
#include "../Common/Packet.h"

class ClientWorker : public QObject {
    Q_OBJECT
public:
    explicit ClientWorker(QObject* parent = nullptr);

public slots:
    void connectAndRun(QString ip, int port, QString username, QString password);
    void sendVote(int votedPlayerId);
    void sendImage(QByteArray pixels, int sessionID, int myPlayerId);

signals:
    void logMessage(const QString& text);
    void loginFailed(const QString& reason);
    void loginSuccess(const QString& username);
    void gameStarted();
    void promptReceived(const QString& prompt, int sessionID, int myPlayerId);
    void drawingReceived(int playerId, const QString& playerName, QByteArray pixels, int width, int height);
    void voteRequestReceived(const QString& playerList);
    void resultsReceived(const QString& results);
    void gameEnded();
    void errorOccurred(const QString& msg);

private:
    SOCKET  serverSocket = INVALID_SOCKET;
    WSADATA wsaData;
    bool    connected = false;
    int     pendingVote = -1;
    bool    voteReady = false;
    bool    imageReady = false;
    int     currentSessionID = -1;

    void sendPacket(const Packet& pkt);
    Packet receivePacket();
    void sendString(const std::string& str);
};