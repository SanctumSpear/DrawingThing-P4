#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#include <ws2tcpip.h>
#include "ClientWindow.h"
#include <QApplication>
#include <QMessageBox>
#include <QPixmap>
#include <QImage>
#include <QLabel>
#include <QPushButton>
#include <QFormLayout>
#include <QScrollArea>
#include <QDateTime>
#include <QFile>
#include <QTextStream>
#include <QFont>
#include <fstream>
#include <iostream>

// -------------------------------------------------------
// LoginPage
// -------------------------------------------------------
LoginPage::LoginPage(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    QGroupBox* box = new QGroupBox("Connect to Game Server");
    box->setFixedWidth(400);
    QFormLayout* form = new QFormLayout(box);
    form->setSpacing(10);

    ipEdit = new QLineEdit("127.0.0.1");
    portEdit = new QLineEdit("54000");
    usernameEdit = new QLineEdit();
    passwordEdit = new QLineEdit();
    passwordEdit->setEchoMode(QLineEdit::Password);

    form->addRow("Server IP:", ipEdit);
    form->addRow("Port:", portEdit);
    form->addRow("Username:", usernameEdit);
    form->addRow("Password:", passwordEdit);

    statusLabel = new QLabel("");
    statusLabel->setStyleSheet("color: red;");
    statusLabel->setAlignment(Qt::AlignCenter);
    form->addRow(statusLabel);

    QHBoxLayout* btnLayout = new QHBoxLayout();
    QPushButton* loginBtn = new QPushButton("Login");
    QPushButton* registerBtn = new QPushButton("Register");
    loginBtn->setMinimumHeight(36);
    registerBtn->setMinimumHeight(36);
    btnLayout->addWidget(loginBtn);
    btnLayout->addWidget(registerBtn);
    form->addRow(btnLayout);

    layout->addWidget(box);

    connect(loginBtn, &QPushButton::clicked, this, &LoginPage::onLoginClicked);
    connect(registerBtn, &QPushButton::clicked, this, &LoginPage::onRegisterClicked);
}

void LoginPage::onLoginClicked() {
    if (usernameEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
        statusLabel->setText("Username and password required.");
        return;
    }
    statusLabel->setText("Connecting...");
    emit loginRequested(
        ipEdit->text(),
        portEdit->text().toInt(),
        usernameEdit->text(),
        passwordEdit->text());
}

void LoginPage::onRegisterClicked() {
    if (usernameEdit->text().isEmpty() || passwordEdit->text().isEmpty()) {
        statusLabel->setText("Username and password required.");
        return;
    }
    emit registerRequested(usernameEdit->text(), passwordEdit->text());
}

// -------------------------------------------------------
// WaitingPage
// -------------------------------------------------------
WaitingPage::WaitingPage(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);

    messageLabel = new QLabel("Waiting...");
    QFont f;
    f.setPointSize(16);
    messageLabel->setFont(f);
    messageLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(messageLabel);
}

void WaitingPage::setMessage(const QString& msg) {
    messageLabel->setText(msg);
}

// -------------------------------------------------------
// VotingPage
// -------------------------------------------------------
VotingPage::VotingPage(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    instructionLabel = new QLabel("Vote for the best drawing! (You cannot vote for yourself)");
    QFont f;
    f.setPointSize(13);
    f.setBold(true);
    instructionLabel->setFont(f);
    instructionLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(instructionLabel);

    scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);

    drawingsContainer = new QWidget();
    drawingsLayout = new QGridLayout(drawingsContainer);
    drawingsLayout->setSpacing(16);

    scrollArea->setWidget(drawingsContainer);
    layout->addWidget(scrollArea);
}

void VotingPage::reset() {
    drawingCount = 0;
    QLayoutItem* item;
    while ((item = drawingsLayout->takeAt(0)) != nullptr) {
        delete item->widget();
        delete item;
    }
}

void VotingPage::addDrawing(int playerId, const QString& playerName,
    QByteArray pixels, int width, int height) {
    if (playerId == myPlayerId) return;

    QWidget* card = new QWidget();
    QVBoxLayout* cardLayout = new QVBoxLayout(card);
    card->setStyleSheet("border: 1px solid #ccc; border-radius: 8px; padding: 8px;");

    QImage img((const uchar*)pixels.constData(), width, height,
        width * 3, QImage::Format_RGB888);
    img = img.flipped(Qt::Vertical);

    QLabel* imgLabel = new QLabel();
    imgLabel->setPixmap(QPixmap::fromImage(img).scaled(
        320, 240, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    imgLabel->setAlignment(Qt::AlignCenter);

    QLabel* nameLabel = new QLabel(playerName);
    nameLabel->setAlignment(Qt::AlignCenter);
    QFont f;
    f.setBold(true);
    nameLabel->setFont(f);

    QPushButton* voteBtn = new QPushButton("Vote for this drawing");
    voteBtn->setMinimumHeight(36);

    int pid = playerId;
    connect(voteBtn, &QPushButton::clicked, this, [this, pid]() {
        emit voteSubmitted(pid);
        });

    cardLayout->addWidget(imgLabel);
    cardLayout->addWidget(nameLabel);
    cardLayout->addWidget(voteBtn);

    int row = drawingCount / 2;
    int col = drawingCount % 2;
    drawingsLayout->addWidget(card, row, col);
    drawingCount++;
}

void VotingPage::setPlayerList(const QString& playerList) {
    Q_UNUSED(playerList);
}

// -------------------------------------------------------
// ResultsPage
// -------------------------------------------------------
ResultsPage::ResultsPage(QWidget* parent) : QWidget(parent) {
    QVBoxLayout* layout = new QVBoxLayout(this);

    titleLabel = new QLabel("Game Results");
    QFont f;
    f.setPointSize(18);
    f.setBold(true);
    titleLabel->setFont(f);
    titleLabel->setAlignment(Qt::AlignCenter);
    layout->addWidget(titleLabel);

    resultsView = new QPlainTextEdit();
    resultsView->setReadOnly(true);
    QFont rf;
    rf.setPointSize(13);
    resultsView->setFont(rf);
    layout->addWidget(resultsView);

    QPushButton* quitBtn = new QPushButton("Close Game");
    quitBtn->setMinimumHeight(40);
    connect(quitBtn, &QPushButton::clicked, qApp, &QApplication::quit);
    layout->addWidget(quitBtn);
}

void ResultsPage::setResults(const QString& results) {
    resultsView->setPlainText(results);
}

// -------------------------------------------------------
// ClientWindow
// -------------------------------------------------------
ClientWindow::ClientWindow(QWidget* parent)
    : QMainWindow(parent),
    workerThread(new QThread(this)),
    worker(new ClientWorker())
{
    setWindowTitle("Drawing Game");
    resize(900, 650);

    stack = new QStackedWidget(this);
    setCentralWidget(stack);

    loginPage = new LoginPage();
    waitingPage = new WaitingPage();
    votingPage = new VotingPage();
    resultsPage = new ResultsPage();

    stack->addWidget(loginPage);    // 0
    stack->addWidget(waitingPage);  // 1
    stack->addWidget(votingPage);   // 2
    stack->addWidget(resultsPage);  // 3

    worker->moveToThread(workerThread);
    workerThread->start();

    connect(loginPage, &LoginPage::loginRequested,
        this, &ClientWindow::onLoginRequested);
    connect(loginPage, &LoginPage::registerRequested,
        this, &ClientWindow::onRegisterRequested);

    connect(worker, &ClientWorker::loginFailed,
        this, &ClientWindow::onLoginFailed, Qt::QueuedConnection);
    connect(worker, &ClientWorker::loginSuccess,
        this, &ClientWindow::onLoginSuccess, Qt::QueuedConnection);
    connect(worker, &ClientWorker::gameStarted,
        this, &ClientWindow::onGameStarted, Qt::QueuedConnection);
    connect(worker, &ClientWorker::promptReceived,
        this, &ClientWindow::onPromptReceived, Qt::QueuedConnection);
    connect(worker, &ClientWorker::drawingReceived,
        this, &ClientWindow::onDrawingReceived, Qt::QueuedConnection);
    connect(worker, &ClientWorker::voteRequestReceived,
        this, &ClientWindow::onVoteRequestReceived, Qt::QueuedConnection);
    connect(worker, &ClientWorker::resultsReceived,
        this, &ClientWindow::onResultsReceived, Qt::QueuedConnection);
    connect(worker, &ClientWorker::gameEnded,
        this, &ClientWindow::onGameEnded, Qt::QueuedConnection);
    connect(worker, &ClientWorker::errorOccurred,
        this, &ClientWindow::onError, Qt::QueuedConnection);

    connect(votingPage, &VotingPage::voteSubmitted,
        this, &ClientWindow::onVoteSubmitted);
}

ClientWindow::~ClientWindow() {
    workerThread->quit();
    workerThread->wait();
    delete worker;
}

void ClientWindow::onLoginRequested(QString ip, int port,
    QString username, QString password) {
    currentUsername = username;
    QMetaObject::invokeMethod(worker, "connectAndRun",
        Qt::QueuedConnection,
        Q_ARG(QString, ip),
        Q_ARG(int, port),
        Q_ARG(QString, username),
        Q_ARG(QString, password));
}

void ClientWindow::onRegisterRequested(QString username, QString password) {
    std::ofstream file("accounts.txt", std::ios::app);
    if (!file.is_open()) {
        QMessageBox::warning(this, "Error", "Could not open accounts.txt");
        return;
    }
    file << username.toStdString() << ":" << password.toStdString() << "\n";
    file.close();
    QMessageBox::information(this, "Registered",
        "Account created for " + username + ".\nYou can now log in.");
}

void ClientWindow::onLoginFailed(const QString& reason) {
    QMessageBox::warning(this, "Login Failed", reason);
    stack->setCurrentIndex(0);
}

void ClientWindow::onLoginSuccess(const QString& username) {
    currentUsername = username;
    waitingPage->setMessage("Logged in as " + username +
        ".\nWaiting for other players...");
    stack->setCurrentIndex(1);
}

void ClientWindow::onGameStarted() {
    waitingPage->setMessage("Game starting!\nWaiting for prompt...");
}

void ClientWindow::onPromptReceived(const QString& prompt,
    int sessionID, int myPlayerId) {
    this->myPlayerId = myPlayerId;
    this->currentSessionID = sessionID;
    votingPage->setMyPlayerId(myPlayerId);

    waitingPage->setMessage(
        "Your prompt:\n\n\"" + prompt + "\"\n\n"
        "The drawing window will open shortly.\n"
        "Press ENTER or SPACE to submit. Press C to clear.");
    stack->setCurrentIndex(1);

    emit launchCanvas(prompt, sessionID, myPlayerId);
}

void ClientWindow::onDrawingReceived(int playerId, const QString& playerName,
    QByteArray pixels, int w, int h) {
    votingPage->addDrawing(playerId, playerName, pixels, w, h);
}

void ClientWindow::onVoteRequestReceived(const QString& playerList) {
    votingPage->setPlayerList(playerList);
    stack->setCurrentIndex(2);
}

void ClientWindow::onVoteSubmitted(int playerId) {
    QMetaObject::invokeMethod(worker, "sendVote",
        Qt::QueuedConnection,
        Q_ARG(int, playerId));
    waitingPage->setMessage("Vote submitted!\nWaiting for results...");
    stack->setCurrentIndex(1);
}

void ClientWindow::onResultsReceived(const QString& results) {
    resultsPage->setResults(results);
    stack->setCurrentIndex(3);
}

void ClientWindow::onGameEnded() {}

void ClientWindow::onError(const QString& msg) {
    QMessageBox::critical(this, "Error", msg);
    stack->setCurrentIndex(0);
}