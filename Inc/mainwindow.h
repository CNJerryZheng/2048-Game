#pragma once

#include <QElapsedTimer>
#include <QMainWindow>
#include <QString>

extern "C" {
#include "board.h"
}

class QLabel;
class QLineEdit;
class QPushButton;
class QStackedWidget;
class QTabWidget;
class QTimer;
class QVBoxLayout;
class BoardWidget;
class QKeyEvent;
class QCloseEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

private:
    void buildAuthPage();
    void buildMenuPage();
    void buildGamePage();
    void buildDetailPage();
    void login();
    void registerUser();
    void enterAsGuest();
    void showMenu();
    void beginNewGame();
    void startSavedGame();
    void saveGame(bool showMessage = true);
    void returnToMenu();
    void logout();
    void undoMove();
    void restartGame();
    void showRanking();
    void showHistory();
    void showUserCenter();
    void showSettings();
    void showHelp();
    void showDetailPage(const QString &title, QWidget *content);
    void renderBoard();
    void updateGameInfo();
    void updateTimeLabel();
    void handleMove(BoardCommand command);
    void finishAnimatedMove();
    void finishGame(BoardStatus status);
    int currentElapsedSeconds() const;
    int autoSaveInterval() const;
    QString authMessage(int result) const;
    QString modeDisplayName(const char *mode) const;

    QStackedWidget *pages = nullptr;
    QWidget *authPage = nullptr;
    QWidget *menuPage = nullptr;
    QWidget *gamePage = nullptr;
    QWidget *detailPage = nullptr;
    QVBoxLayout *detailLayout = nullptr;
    QTabWidget *authTabs = nullptr;
    QLineEdit *loginUsername = nullptr;
    QLineEdit *loginPassword = nullptr;
    QLabel *loginMessage = nullptr;
    QLineEdit *registerUsername = nullptr;
    QLineEdit *registerPassword = nullptr;
    QLineEdit *registerConfirmation = nullptr;
    QLabel *registerMessage = nullptr;
    QLabel *welcomeLabel = nullptr;
    QPushButton *continueButton = nullptr;
    QPushButton *userCenterButton = nullptr;
    QPushButton *historyButton = nullptr;
    QPushButton *saveButton = nullptr;
    QPushButton *undoButton = nullptr;
    QLabel *scoreLabel = nullptr;
    QLabel *stepLabel = nullptr;
    QLabel *timeLabel = nullptr;
    QLabel *modeLabel = nullptr;
    QLabel *gameMessage = nullptr;
    BoardWidget *boardWidget = nullptr;
    QTimer *gameTimer = nullptr;
    QElapsedTimer sessionTimer;
    QString currentUser;
    Board board = {};
    Board undoBoard = {};
    bool gameActive = false;
    bool guestMode = false;
    bool canUndo = false;
    bool hasQueuedMove = false;
    BoardCommand queuedMove = BOARD_CMD_UP;
};
