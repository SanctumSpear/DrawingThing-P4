#pragma once

#include <QMainWindow>
#include <QObject>
#include <QPixmap>
#include <QVector>

class QLabel;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QThread;
class QStackedWidget;
class QScrollArea;
class QWidget;

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

    // Emitted when a player's raw GL_RGB image is received from the client.
    // pixels: GL_RGB / GL_UNSIGNED_BYTE data, bottom-to-top row order.
    void imageReceived(int playerId, const QString& playerName,
                       const QByteArray& pixels, int width, int height);
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
    void onViewDrawingsClicked();

    void onServerStarted();
    void onServerStopped();
    void onServerError(const QString& text);

    void appendLog(const QString& text);
    void setState(const QString& text);
    void addPlayer(const QString& name);
    void clearPlayers();

    // Receives a raw OpenGL image from the worker thread,
    // flips it vertically, and stores it as a QPixmap.
    void storeImage(int playerId, const QString& playerName,
                    const QByteArray& pixels, int width, int height);

private:
    void setupUi();
    void setupConnections();
    void loadLogFile();
    void rebuildImagePanel();   // repopulates imageContainer with stored images

private:
    QLabel* serverStatusLabel;
    QLabel* gameStateLabel;
    QLabel* maxPlayersLabel;

    QSpinBox* maxPlayersSpin;

    QListWidget* playersList;
    QPlainTextEdit* logsView;

    // Right-panel stack: page 0 = log, page 1 = image gallery
    QStackedWidget* logStack;
    QWidget*        imageContainer; // lives inside the scroll area on page 1

    QPushButton* startButton;
    QPushButton* refreshLogsButton;
    QPushButton* viewDrawingsButton;
    QPushButton* quitButton;

    QThread*      workerThread;
    ServerWorker* worker;

    // Received player drawings (converted from raw GL pixels)
    struct PlayerImage {
        int     id;
        QString name;
        QPixmap pixmap;
    };
    QVector<PlayerImage> storedImages;
};