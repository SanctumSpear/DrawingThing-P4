#pragma once

#include <QMainWindow>
#include <QObject>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QThread;

class ServerWorker : public QObject
{
    Q_OBJECT

public:
    explicit ServerWorker(QObject* parent = nullptr);

public slots:
    void runServer(int maxPlayers);

signals:
    void logMessage(const QString& text);
    void stateChanged(const QString& text);
    void playerJoined(const QString& name);
    void playerListCleared();
    void serverStarted();
    void serverStopped();
    void serverError(const QString& text);
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onStartClicked();
    void onRefreshLogsClicked();

    void onServerStarted();
    void onServerStopped();
    void onServerError(const QString& text);

    void appendLog(const QString& text);
    void setState(const QString& text);
    void addPlayer(const QString& name);
    void clearPlayers();

private:
    void setupUi();
    void setupConnections();
    void loadLogFile();

private:
    QLabel* serverStatusLabel;
    QLabel* gameStateLabel;
    QLabel* maxPlayersLabel;

    QSpinBox* maxPlayersSpin;

    QListWidget* playersList;
    QPlainTextEdit* logsView;

    QPushButton* startButton;
    QPushButton* refreshLogsButton;
    QPushButton* quitButton;

    QThread* workerThread;
    ServerWorker* worker;
};