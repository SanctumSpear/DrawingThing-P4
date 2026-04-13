#include "MainWindow.h"

#include <QWidget>
#include <QLabel>
#include <QListWidget>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QThread>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGroupBox>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <QMessageBox>
#include <QString>
#include <QStackedWidget>
#include <QScrollArea>
#include <QImage>
#include <QPixmap>
#include <cstring>

#include <iostream>
#include <string>
#include <vector>
#include <random>
#include <fstream>

#include "Communication.h"
#include "AccountManager.h"
#include "../Common/Game.h"
#include "../Common/Packet.h"
#include "../Common/logger.h"

static void EnterState(Game& game, GameState s, Logger& log)
{
    game.ChangeState(s);

    std::string name;
    switch (s)
    {
    case GameState::STARTUP:   name = "STARTUP";   break;
    case GameState::WAITING:   name = "WAITING";   break;
    case GameState::SENDING:   name = "SENDING";   break;
    case GameState::RECEIVING: name = "RECEIVING"; break;
    case GameState::VOTING:    name = "VOTING";    break;
    case GameState::ENDING:    name = "ENDING";    break;
    }

    std::cout << "[STATE] -> " << name << "\n";
    log.Log(SC_XSTATE, game.GetSessionID(), true, name);
}

ServerWorker::ServerWorker(QObject* parent)
    : QObject(parent)
{
}

void ServerWorker::runServer(int maxPlayers)
{
    try
    {
        const int MAX_PLAYERS = maxPlayers;
        const int SESSION_ID = 1;

        Logger log("server_log.txt");
        AccountManager accounts("accounts.txt");
        Server server(54000);
        Game game((uint8_t)SESSION_ID);

        emit playerListCleared();
        emit serverStarted();
        emit logMessage("Server listening on port 54000...");

        EnterState(game, GameState::STARTUP, log);
        emit stateChanged("STARTUP");

        EnterState(game, GameState::WAITING, log);
        emit stateChanged("WAITING");
        emit logMessage(QString("Waiting for %1 player(s)...").arg(MAX_PLAYERS));

        while (game.GetPlayerCount() < MAX_PLAYERS)
        {
            int sockIdx = server.AcceptClient();
            emit logMessage(QString("Connection accepted (socket %1).").arg(sockIdx));

            log.Log(SC_CONNECT, game.GetSessionID(), true,
                "socket " + std::to_string(sockIdx));

            std::string credentials = server.ReceiveString(sockIdx);
            emit logMessage("Credentials received.");

            log.Log(SR_RECV, game.GetSessionID(), true,
                "credentials from socket " + std::to_string(sockIdx));

            // Detect a registration request vs. a normal login.
            const std::string REG_PREFIX = "REGISTER:";
            bool isRegister = (credentials.rfind(REG_PREFIX, 0) == 0);
            if (isRegister)
                credentials = credentials.substr(REG_PREFIX.size());

            auto colonPos = credentials.find(':');
            if (colonPos == std::string::npos)
            {
                Packet err = Packet::MakeErrorPacket(
                    game.GetSessionID(),
                    "Bad credentials format",
                    0,
                    (uint8_t)(sockIdx + 1));

                server.SendPacket(sockIdx, err);
                log.Log(SL_LOGIN, game.GetSessionID(), false, "malformed credentials");
                emit logMessage("Bad credentials format. Client rejected.");
                server.CloseClient(sockIdx);
                continue;
            }

            std::string username = credentials.substr(0, colonPos);
            std::string password = credentials.substr(colonPos + 1);

            // --- Registration branch ---
            if (isRegister)
            {
                if (!accounts.CreateAccount(username, password))
                {
                    // Username already exists
                    Packet err = Packet::MakeErrorPacket(
                        game.GetSessionID(),
                        "Username already taken",
                        0, 0);
                    server.SendPacket(sockIdx, err);

                    std::cout << "  Register FAILED (taken): \"" << username << "\"\n";
                    emit logMessage(QString("Register FAILED (taken): \"%1\".")
                        .arg(QString::fromStdString(username)));
                    log.Log(SL_LOGIN, game.GetSessionID(), false,
                        "register failed (taken): " + username);
                }
                else
                {
                    Packet ack = Packet::MakeAckPacket(game.GetSessionID(), 0, 0);
                    server.SendPacket(sockIdx, ack);

                    std::cout << "  Registered: \"" << username << "\"\n";
                    emit logMessage(QString("Registered: \"%1\". They must log in to join.")
                        .arg(QString::fromStdString(username)));
                    log.Log(SL_LOGIN, game.GetSessionID(), true,
                        "registered: " + username);
                }

                // Close the registration connection — player must reconnect to login.
                server.CloseClient(sockIdx);
                continue;
            }

            // --- Login branch (unchanged) ---
            if (!accounts.Authenticate(username, password))
            {
                Packet err = Packet::MakeErrorPacket(
                    game.GetSessionID(),
                    "Authentication failed",
                    0,
                    (uint8_t)(sockIdx + 1));

                server.SendPacket(sockIdx, err);

                std::cout << "  Auth FAILED for \"" << username << "\"\n";
                emit logMessage(QString("Auth FAILED for \"%1\".")
                    .arg(QString::fromStdString(username)));

                log.Log(SL_LOGIN, game.GetSessionID(), false, username);
                server.CloseClient(sockIdx);
                continue;
            }

            int playerId = game.GetPlayerCount() + 1;
            game.AddPlayer(username, playerId, sockIdx);

            Packet ack = Packet::MakeAckPacket(
                game.GetSessionID(), 0, (uint8_t)playerId);
            server.SendPacket(sockIdx, ack);

            std::cout << "  Auth OK: " << username
                << " -> Player " << playerId << "\n\n";

            emit playerJoined(QString::fromStdString(username));
            emit logMessage(QString("Auth OK: %1 -> Player %2")
                .arg(QString::fromStdString(username))
                .arg(playerId));

            log.Log(SL_LOGIN, game.GetSessionID(), true, username);
        }

        game.PrintPlayers();
        emit logMessage("All players connected.");

        EnterState(game, GameState::SENDING, log);
        emit stateChanged("SENDING");

        Packet startPkt = Packet::MakeGameStartPacket(game.GetSessionID(), 0, 0);
        server.BroadcastPacket(startPkt);
        emit logMessage("GAME_START broadcast.");
        log.Log(SS_SEND, game.GetSessionID(), true, "GAME_START broadcast");

        std::vector<std::string> PROMPTS;
        {
            std::ifstream promptFile("prompts.txt");
            if (promptFile.is_open())
            {
                std::string line;
                while (std::getline(promptFile, line))
                    if (!line.empty())
                        PROMPTS.push_back(line);
            }
            if (PROMPTS.empty())
            {
                emit logMessage("[WARN] prompts.txt not found or empty — using fallback prompt.");
                PROMPTS.push_back("A Dog riding a unicycle");
            }
        }

        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<size_t> dist(0, PROMPTS.size() - 1);
        game.SetPrompt(PROMPTS[dist(rng)]);

        emit logMessage(QString("Prompt selected: \"%1\"")
            .arg(QString::fromStdString(game.GetPrompt())));
        log.Log(SS_PRMT, game.GetSessionID(), true,
            "selected prompt: " + game.GetPrompt());

        for (int i = 0; i < game.GetPlayerCount(); i++)
        {
            const Player& p = game.GetPlayer(i);
            Packet promptPkt = Packet::MakePromptPacket(
                game.GetSessionID(),
                game.GetPrompt(),
                0,
                (uint8_t)p.GetId());

            server.SendPacket(p.GetSocketIndex(), promptPkt);

            std::cout << "PROMPT sent to " << p.GetName()
                << ": \"" << game.GetPrompt() << "\"\n";

            emit logMessage(QString("PROMPT sent to %1: \"%2\"")
                .arg(QString::fromStdString(p.GetName()))
                .arg(QString::fromStdString(game.GetPrompt())));

            log.Log(SS_PRMT, game.GetSessionID(), true,
                "prompt -> " + p.GetName());
        }

        EnterState(game, GameState::WAITING, log);
        emit stateChanged("WAITING");
        emit logMessage("Waiting for ACKs from all players...");

        for (int i = 0; i < game.GetPlayerCount(); i++)
        {
            const Player& p = game.GetPlayer(i);
            Packet ack = server.ReceivePacket(p.GetSocketIndex());

            if (ack.header.type == PacketType::ACK)
            {
                std::cout << "  ACK from " << p.GetName() << ".\n";

                emit logMessage(QString("ACK from %1.")
                    .arg(QString::fromStdString(p.GetName())));

                log.Log(SR_RECV, game.GetSessionID(), true,
                    "ACK from " + p.GetName());
            }
        }

        EnterState(game, GameState::RECEIVING, log);
        emit stateChanged("RECEIVING");
        emit logMessage("Waiting for drawings from all players...");

        for (int i = 0; i < game.GetPlayerCount(); i++)
        {
            const Player& p = game.GetPlayer(i);

            std::cout << "  Waiting for image from " << p.GetName() << "...\n";
            emit logMessage(QString("Waiting for image from %1...")
                .arg(QString::fromStdString(p.GetName())));

            Packet imgPkt = server.ReceivePacket(p.GetSocketIndex());
            if (imgPkt.header.type == PacketType::IMAGE)
            {
                bool crcOk = imgPkt.ValidateCRC();

                game.StorePlayerImage(
                    p.GetId(),
                    imgPkt.data,
                    imgPkt.header.payloadSize);

                std::cout << "  IMAGE from " << p.GetName()
                    << " (" << imgPkt.header.payloadSize
                    << " bytes, CRC " << (crcOk ? "OK" : "FAIL") << ").\n";

                emit logMessage(QString("IMAGE from %1 (%2 bytes, CRC %3).")
                    .arg(QString::fromStdString(p.GetName()))
                    .arg(imgPkt.header.payloadSize)
                    .arg(crcOk ? "OK" : "FAIL"));

                log.Log(SR_JPEG, game.GetSessionID(), crcOk,
                    std::to_string(imgPkt.header.payloadSize)
                    + " bytes from " + p.GetName());

                // Forward raw GL_RGB pixels to the UI (800×600, bottom-to-top).
                constexpr int IMG_W = 800, IMG_H = 600;
                if ((int)imgPkt.header.payloadSize == IMG_W * IMG_H * 3)
                {
                    emit imageReceived(
                        p.GetId(),
                        QString::fromStdString(p.GetName()),
                        QByteArray(imgPkt.data, (int)imgPkt.header.payloadSize),
                        IMG_W, IMG_H);
                }
            }
        }

        std::cout << "All drawings received.\n\n";
        emit logMessage("All drawings received.");

        EnterState(game, GameState::VOTING, log);
        emit stateChanged("VOTING");

        for (int i = 0; i < game.GetPlayerCount(); i++)
        {
            const Player& p = game.GetPlayer(i);
            const auto& imgData = game.GetPlayerImage(p.GetId());

            Packet imgBcast = Packet::MakeImagePacket(
                game.GetSessionID(),
                imgData.data(),
                (uint32_t)imgData.size(),
                (uint8_t)p.GetId(),
                0);

            server.BroadcastPacket(imgBcast);

            std::cout << "Broadcast " << p.GetName() << "'s drawing ("
                << imgData.size() << " bytes) to all players.\n";

            emit logMessage(QString("Broadcast %1's drawing (%2 bytes) to all players.")
                .arg(QString::fromStdString(p.GetName()))
                .arg((int)imgData.size()));

            log.Log(SS_JPEG, game.GetSessionID(), true,
                "broadcast drawing from " + p.GetName());
        }

        std::string playerList = game.GetVoteRequestString();
        Packet voteReqPkt = Packet::MakeVoteRequestPacket(
            game.GetSessionID(), playerList, 0, 0);
        server.BroadcastPacket(voteReqPkt);

        std::cout << "VOTE_REQUEST broadcast.\n";
        emit logMessage("VOTE_REQUEST broadcast.");

        log.Log(SS_SEND, game.GetSessionID(), true, "VOTE_REQUEST broadcast");

        std::cout << "\nCollecting votes...\n";
        emit logMessage("Collecting votes...");

        for (int i = 0; i < game.GetPlayerCount(); i++)
        {
            const Player& p = game.GetPlayer(i);
            Packet votePkt = server.ReceivePacket(p.GetSocketIndex());

            if (votePkt.header.type == PacketType::VOTE)
            {
                uint8_t votedId = votePkt.GetVotedPlayerId();

                if ((int)votedId == p.GetId())
                {
                    std::cout << "  " << p.GetName()
                        << " tried to vote for themselves - ignored.\n";

                    emit logMessage(QString("%1 tried to vote for themselves - ignored.")
                        .arg(QString::fromStdString(p.GetName())));

                    log.Log(SR_RECV, game.GetSessionID(), false,
                        p.GetName() + " self-vote rejected");
                }
                else
                {
                    game.RecordVote(p.GetId(), (int)votedId);

                    emit logMessage(QString("%1 voted for player %2.")
                        .arg(QString::fromStdString(p.GetName()))
                        .arg((int)votedId));

                    log.Log(SR_RECV, game.GetSessionID(), true,
                        p.GetName() + " voted for player " + std::to_string(votedId));
                }
            }
        }

        // Award 50 points per vote received to every player, then identify winner.
        int winnerId = game.AwardVotePoints(50);
        if (winnerId != -1)
        {
            const Player* winner = game.FindPlayerById(winnerId);
            std::string winnerName = winner ? winner->GetName() : "Player " + std::to_string(winnerId);

            std::cout << "\nWinner: " << winnerName << " had the most votes!\n";

            emit logMessage(QString("Winner: %1 had the most votes!")
                .arg(QString::fromStdString(winnerName)));

            log.Log(SS_SEND, game.GetSessionID(), true,
                "awarded vote-based points; winner: " + winnerName);
        }
        else
        {
            std::cout << "\nNo votes cast - no winner.\n";
            emit logMessage("No votes cast - no winner.");
        }

        game.PrintPlayers();

        EnterState(game, GameState::SENDING, log);
        emit stateChanged("SENDING");

        std::string resultsStr = game.GetResultsString();
        Packet resultsPkt = Packet::MakeResultsPacket(
            game.GetSessionID(), resultsStr, 0, 0);
        server.BroadcastPacket(resultsPkt);

        std::cout << "RESULTS broadcast:\n" << resultsStr;
        emit logMessage("RESULTS broadcast.");
        emit logMessage(QString::fromStdString(resultsStr));

        log.Log(SS_SEND, game.GetSessionID(), true, "RESULTS broadcast");

        EnterState(game, GameState::ENDING, log);
        emit stateChanged("ENDING");

        Packet endPkt = Packet::MakeGameEndPacket(game.GetSessionID(), 0, 0);
        server.BroadcastPacket(endPkt);

        std::cout << "GAME_END broadcast.\n";
        emit logMessage("GAME_END broadcast.");

        log.Log(SS_SEND, game.GetSessionID(), true, "GAME_END broadcast");

        game.ChangeProgramRunning(false);

        for (int i = 0; i < game.GetPlayerCount(); i++)
        {
            log.Log(SC_DISCONNECT, game.GetSessionID(), true,
                game.GetPlayer(i).GetName());
        }

        std::cout << "\nGame session ended.\n";
        emit logMessage("Game session ended.");

        server.Cleanup();
        emit serverStopped();
    }
    catch (const std::exception& ex)
    {
        emit serverError(QString("Server error: %1").arg(ex.what()));
        emit serverStopped();
    }
    catch (...)
    {
        emit serverError("Unknown server error.");
        emit serverStopped();
    }
}

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent),
    serverStatusLabel(nullptr),
    gameStateLabel(nullptr),
    maxPlayersLabel(nullptr),
    maxPlayersSpin(nullptr),
    playersList(nullptr),
    logsView(nullptr),
    startButton(nullptr),
    refreshLogsButton(nullptr),
    quitButton(nullptr),
    workerThread(new QThread(this)),
    worker(new ServerWorker())
{
    setupUi();
    setupConnections();

    worker->moveToThread(workerThread);
    workerThread->start();

    serverStatusLabel->setText("Server: Ready");
    gameStateLabel->setText("State: IDLE");
    appendLog("UI ready.");
    loadLogFile();
}

MainWindow::~MainWindow()
{
    workerThread->quit();
    workerThread->wait();
    delete worker;
}

void MainWindow::setupUi()
{
    setWindowTitle("Drawing Game Server");
    resize(1100, 700);

    QWidget* centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    QVBoxLayout* mainLayout = new QVBoxLayout(centralWidget);
    mainLayout->setContentsMargins(16, 16, 16, 16);
    mainLayout->setSpacing(12);

    // ── Top bar: status labels + max players ──────────────────────────────────
    QHBoxLayout* topLayout = new QHBoxLayout();

    serverStatusLabel = new QLabel("Server: Ready");
    gameStateLabel    = new QLabel("State: IDLE");
    maxPlayersLabel   = new QLabel("Max Players:");
    maxPlayersSpin    = new QSpinBox();

    maxPlayersSpin->setMinimum(1);
    maxPlayersSpin->setMaximum(8);
    maxPlayersSpin->setValue(1);

    QFont statusFont;
    statusFont.setPointSize(11);
    statusFont.setBold(true);
    serverStatusLabel->setFont(statusFont);
    gameStateLabel->setFont(statusFont);

    topLayout->addWidget(serverStatusLabel);
    topLayout->addSpacing(20);
    topLayout->addWidget(gameStateLabel);
    topLayout->addStretch();
    topLayout->addWidget(maxPlayersLabel);
    topLayout->addWidget(maxPlayersSpin);

    mainLayout->addLayout(topLayout);

    // ── Middle: players list (left) + stacked log/image panel (right) ─────────
    QHBoxLayout* middleLayout = new QHBoxLayout();
    middleLayout->setSpacing(12);

    QGroupBox* playersBox = new QGroupBox("Connected / Authenticated Players");
    QVBoxLayout* playersLayout = new QVBoxLayout(playersBox);
    playersList = new QListWidget();
    playersLayout->addWidget(playersList);

    // Right panel: QStackedWidget
    //   Page 0 – plain-text server log
    //   Page 1 – horizontally scrollable drawing gallery
    logStack = new QStackedWidget();

    logsView = new QPlainTextEdit();
    logsView->setReadOnly(true);
    logStack->addWidget(logsView);   // index 0

    QScrollArea* imageScrollArea = new QScrollArea();
    imageScrollArea->setWidgetResizable(true);
    imageScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    imageScrollArea->setStyleSheet("QScrollArea { background: #1e1e1e; }");

    imageContainer = new QWidget();
    imageContainer->setStyleSheet("background: #1e1e1e;");
    imageScrollArea->setWidget(imageContainer);

    logStack->addWidget(imageScrollArea);  // index 1

    QGroupBox* rightBox = new QGroupBox("Server Output");
    QVBoxLayout* rightLayout = new QVBoxLayout(rightBox);
    rightLayout->addWidget(logStack);

    middleLayout->addWidget(playersBox, 1);
    middleLayout->addWidget(rightBox,   2);

    mainLayout->addLayout(middleLayout);

    // ── Bottom: buttons ────────────────────────────────────────────────────────
    QHBoxLayout* bottomLayout = new QHBoxLayout();

    refreshLogsButton   = new QPushButton("Refresh Log File");
    viewDrawingsButton  = new QPushButton("View Drawings");
    startButton         = new QPushButton("Start Server");
    quitButton          = new QPushButton("Quit");

    for (QPushButton* btn : { refreshLogsButton, viewDrawingsButton,
                               startButton, quitButton })
        btn->setMinimumHeight(40);

    viewDrawingsButton->setEnabled(false);

    bottomLayout->addStretch();
    bottomLayout->addWidget(refreshLogsButton);
    bottomLayout->addWidget(viewDrawingsButton);
    bottomLayout->addWidget(startButton);
    bottomLayout->addWidget(quitButton);

    mainLayout->addLayout(bottomLayout);
}

void MainWindow::setupConnections()
{
    connect(startButton,        &QPushButton::clicked, this, &MainWindow::onStartClicked);
    connect(refreshLogsButton,  &QPushButton::clicked, this, &MainWindow::onRefreshLogsClicked);
    connect(viewDrawingsButton, &QPushButton::clicked, this, &MainWindow::onViewDrawingsClicked);
    connect(quitButton,         &QPushButton::clicked, this, &MainWindow::close);

    connect(worker, &ServerWorker::logMessage,     this, &MainWindow::appendLog);
    connect(worker, &ServerWorker::stateChanged,   this, &MainWindow::setState);
    connect(worker, &ServerWorker::playerJoined,   this, &MainWindow::addPlayer);
    connect(worker, &ServerWorker::playerListCleared, this, &MainWindow::clearPlayers);
    connect(worker, &ServerWorker::serverStarted,  this, &MainWindow::onServerStarted);
    connect(worker, &ServerWorker::serverStopped,  this, &MainWindow::onServerStopped);
    connect(worker, &ServerWorker::serverError,    this, &MainWindow::onServerError);

    connect(worker, &ServerWorker::imageReceived,  this, &MainWindow::storeImage);
}

void MainWindow::onStartClicked()
{
    startButton->setEnabled(false);
    maxPlayersSpin->setEnabled(false);

    appendLog(QString("Starting server with max players = %1...")
        .arg(maxPlayersSpin->value()));

    QMetaObject::invokeMethod(
        worker,
        "runServer",
        Qt::QueuedConnection,
        Q_ARG(int, maxPlayersSpin->value())
    );
}

void MainWindow::onRefreshLogsClicked()
{
    logStack->setCurrentIndex(0);
    viewDrawingsButton->setText("View Drawings");
    loadLogFile();
}

void MainWindow::onServerStarted()
{
    serverStatusLabel->setText("Server: Running");

    storedImages.clear();
    viewDrawingsButton->setEnabled(false);
    viewDrawingsButton->setText("View Drawings");
    logStack->setCurrentIndex(0);
}

void MainWindow::onServerStopped()
{
    serverStatusLabel->setText("Server: Stopped");
    startButton->setEnabled(true);
    maxPlayersSpin->setEnabled(true);
}

void MainWindow::onServerError(const QString& text)
{
    appendLog(text);
    QMessageBox::warning(this, "Server Error", text);
}

void MainWindow::appendLog(const QString& text)
{
    QString stamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    logsView->appendPlainText(stamp + text);
}

void MainWindow::setState(const QString& text)
{
    gameStateLabel->setText("State: " + text);
}

void MainWindow::addPlayer(const QString& name)
{
    playersList->addItem(name);
}

void MainWindow::clearPlayers()
{
    playersList->clear();
}

// ── Image handling ─────────────────────────────────────────────────────────────

void MainWindow::storeImage(int playerId, const QString& playerName,
                             const QByteArray& pixels, int width, int height)
{
    if (pixels.size() != width * height * 3)
    {
        appendLog(QString("[WARN] Image from %1 has unexpected size (%2 bytes), skipping.")
            .arg(playerName).arg(pixels.size()));
        return;
    }

    // OpenGL stores rows bottom-to-top; flip vertically so Qt displays it upright.
    QImage img(width, height, QImage::Format_RGB888);
    const uchar* src = reinterpret_cast<const uchar*>(pixels.constData());
    const int rowBytes = width * 3;
    for (int y = 0; y < height; ++y)
    {
        const uchar* srcRow = src + (height - 1 - y) * rowBytes;
        std::memcpy(img.scanLine(y), srcRow, rowBytes);
    }

    PlayerImage pi;
    pi.id     = playerId;
    pi.name   = playerName;
    pi.pixmap = QPixmap::fromImage(img);
    storedImages.append(pi);

    viewDrawingsButton->setEnabled(true);
    appendLog(QString("Drawing from %1 stored — click 'View Drawings' to see it.")
        .arg(playerName));
}

void MainWindow::rebuildImagePanel()
{
    QLayout* old = imageContainer->layout();
    if (old)
    {
        QLayoutItem* item;
        while ((item = old->takeAt(0)) != nullptr)
        {
            if (item->widget())  item->widget()->deleteLater();
            if (item->layout())  {
                QLayoutItem* sub;
                while ((sub = item->layout()->takeAt(0)) != nullptr) {
                    if (sub->widget()) sub->widget()->deleteLater();
                    delete sub;
                }
                delete item->layout();
            }
            delete item;
        }
        delete old;
    }

    QHBoxLayout* layout = new QHBoxLayout(imageContainer);
    layout->setSpacing(20);
    layout->setContentsMargins(16, 16, 16, 16);
    layout->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    if (storedImages.isEmpty())
    {
        QLabel* none = new QLabel("No drawings received yet.");
        none->setAlignment(Qt::AlignCenter);
        none->setStyleSheet("color: #aaa; font-size: 14px;");
        layout->addWidget(none);
    }
    else
    {
        for (const PlayerImage& pi : storedImages)
        {
            QWidget* card = new QWidget();
            card->setStyleSheet(
                "background: #2d2d2d; border-radius: 6px; padding: 8px;");
            QVBoxLayout* col = new QVBoxLayout(card);
            col->setSpacing(6);
            col->setContentsMargins(10, 10, 10, 10);

            QLabel* nameLabel = new QLabel(
                QString("Player %1: %2").arg(pi.id).arg(pi.name));
            nameLabel->setAlignment(Qt::AlignCenter);
            QFont f = nameLabel->font();
            f.setBold(true);
            f.setPointSize(10);
            nameLabel->setFont(f);
            nameLabel->setStyleSheet("color: #fff;");

            QLabel* imgLabel = new QLabel();
            imgLabel->setPixmap(
                pi.pixmap.scaled(400, 300, Qt::KeepAspectRatio,
                                          Qt::SmoothTransformation));
            imgLabel->setAlignment(Qt::AlignCenter);
            imgLabel->setStyleSheet(
                "border: 2px solid #555; background: white;");

            col->addWidget(nameLabel);
            col->addWidget(imgLabel);
            layout->addWidget(card);
        }
        layout->addStretch();
    }
}

void MainWindow::onViewDrawingsClicked()
{
    if (logStack->currentIndex() == 1)
    {
        logStack->setCurrentIndex(0);
        viewDrawingsButton->setText("View Drawings");
    }
    else
    {
        rebuildImagePanel();
        logStack->setCurrentIndex(1);
        viewDrawingsButton->setText("Back to Log");
    }
}

void MainWindow::loadLogFile()
{
    QFile file("server_log.txt");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        appendLog("server_log.txt not found yet.");
        return;
    }

    logsView->clear();

    QTextStream in(&file);
    while (!in.atEnd())
    {
        logsView->appendPlainText(in.readLine());
    }
}                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      