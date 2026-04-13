#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ClientWorker.h"
#include <QMainWindow>
#include <QStackedWidget>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QScrollArea>
#include <QGridLayout>
#include "../Common/Packet.h"

class LoginPage : public QWidget {
    Q_OBJECT
public:
    explicit LoginPage(QWidget* parent = nullptr);

signals:
    void loginRequested(QString ip, int port, QString username, QString password);
    void registerRequested(QString username, QString password);

private slots:
    void onLoginClicked();
    void onRegisterClicked();

private:
    QLineEdit* ipEdit;
    QLineEdit* portEdit;
    QLineEdit* usernameEdit;
    QLineEdit* passwordEdit;
    QLabel* statusLabel;
};

class WaitingPage : public QWidget {
    Q_OBJECT
public:
    explicit WaitingPage(QWidget* parent = nullptr);
    void setMessage(const QString& msg);

private:
    QLabel* messageLabel;
};

class VotingPage : public QWidget {
    Q_OBJECT
public:
    explicit VotingPage(QWidget* parent = nullptr);
    void setMyPlayerId(int id) { myPlayerId = id; }
    void addDrawing(int playerId, const QString& playerName, QByteArray pixels, int width, int height);
    void setPlayerList(const QString& playerList);
    void reset();

signals:
    void voteSubmitted(int playerId);

private:
    int          myPlayerId = -1;
    QGridLayout* drawingsLayout;
    QWidget* drawingsContainer;
    QScrollArea* scrollArea;
    QLabel* instructionLabel;
    int          drawingCount = 0;
};

class ResultsPage : public QWidget {
    Q_OBJECT
public:
    explicit ResultsPage(QWidget* parent = nullptr);
    void setResults(const QString& results);

private:
    QPlainTextEdit* resultsView;
    QLabel* titleLabel;
};

class ClientWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit ClientWindow(QWidget* parent = nullptr);
    ~ClientWindow();
    ClientWorker* worker;

signals:
    void launchCanvas(QString prompt, int sessionID, int myPlayerId);

private slots:
    void onLoginRequested(QString ip, int port, QString username, QString password);
    void onRegisterRequested(QString username, QString password);
    void onLoginFailed(const QString& reason);
    void onLoginSuccess(const QString& username);
    void onGameStarted();
    void onPromptReceived(const QString& prompt, int sessionID, int myPlayerId);
    void onDrawingReceived(int playerId, const QString& playerName, QByteArray pixels, int w, int h);
    void onVoteRequestReceived(const QString& playerList);
    void onVoteSubmitted(int playerId);
    void onResultsReceived(const QString& results);
    void onGameEnded();
    void onError(const QString& msg);

private:
    QStackedWidget* stack;
    LoginPage* loginPage;
    WaitingPage* waitingPage;
    VotingPage* votingPage;
    ResultsPage* resultsPage;
    QThread* workerThread;
    QString         currentUsername;
    int             myPlayerId = -1;
    int             currentSessionID = -1;
};