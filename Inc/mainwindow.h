#pragma once

#include <QElapsedTimer>
#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <QByteArray>

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
class QEvent;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void closeEvent(QCloseEvent *event) override;
    bool eventFilter(QObject *watched, QEvent *event) override;

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
    void clearSessionPassword();
    void undoMove();
    void restartGame();
    void showRanking();
    void showPublicProfile(const QString &username);
    void showHistory();
    void showUserCenter();
    void showEditProfile();
    void showAccountSettings();
    void showPasswordSettings();
    void showClearHistory();
    void showDeleteAccount();
    void showSettings();
    void showDlcManager();
    void showAddDlcKey();
    void showAdminDashboard();
    void showAdminManagement();
    void showAdminDeleteUser();
    void showSystemSettings();
    void showAdminBanUser();
    void showIssueDlcKey();
    void showHelp();
    void showDetailPage(const QString &title, QWidget *content,
                        void (MainWindow::*backAction)() = nullptr);
    void renderBoard();
    void updateGameInfo();
    void updateTimeLabel();
    void handleMove(BoardCommand command);
    void finishAnimatedMove();
    void finishGame(BoardStatus status);
    int currentElapsedSeconds() const;
    int autoSaveInterval() const;
    int defaultAutoSaveInterval() const;
    void loadKeyBindings();
    bool handleConfiguredKey(QKeyEvent *event);
    QString movementKeyText() const;
    QString settingsFilePath() const;
    QString userDirectoryPath() const;
    QString userDirectoryForUsername(const QString &username) const;
    QString userUid(const QString &username) const;
    QString usernameForUid(const QString &uid) const;
    QString ensureUserUid(const QString &username) const;
    QString accountStatus(const QString &username) const;
    QString accountStatusText(const QString &username, bool deleted = false) const;
    void setAccountStatus(const QString &username, const QString &status) const;
    QString saveFilePath() const;
    QString historyFilePath() const;
    QString profileFilePath() const;
    QString modDirectoryPath() const;
    QString systemSettingsPath() const;
    void migrateLegacyUserData();
    QStringList enabledModeIds() const;
    QStringList authorizedDlcIds() const;
    QStringList activatedDlcIds() const;
    bool isDlcAuthorized(const QString &modeId) const;
    bool isDlcActivated(const QString &modeId) const;
    QString dlcDirectoryPath(const QString &uid) const;
    QString dlcFilePath(const QString &uid, const QString &modeId) const;
    QString dlcIssuedListPath() const;
    QString dlcKeyHash(const QString &dlcName, const QString &uid, const QString &key) const;
    QString generateDlcKey(const QString &uid, const QString &modeId) const;
    bool grantDlcKey(const QString &username, const QString &modeId, QString *plainKey = nullptr) const;
    bool activateDlcKey(const QString &uid, const QString &modeId, const QString &dlcName, const QString &plainKey) const;
    QString selectedModeId() const;
    void cycleGameMode();
    void updateModeButton();
    bool isConfiguredAdmin(const QString &username) const;
    bool isSuperAdminUser(const QString &username) const;
    bool isAdminUser(const QString &username) const;
    bool isCurrentUserBanned() const;
    bool canOpenAdminSettings() const;
    QString displayNameForUser(const QString &username) const;
    void deleteUser(const QString &username);
    int usernameCooldownSeconds() const;
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
    QPushButton *modeButton = nullptr;
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
    QByteArray sessionPassword;
    Board board = {};
    Board undoBoard = {};
    bool gameActive = false;
    bool guestMode = false;
    bool adminMode = false;
    bool canUndo = false;
    bool hasUnsavedChanges = false;
    bool hasQueuedMove = false;
    BoardCommand queuedMove = BOARD_CMD_UP;
    int moveUpKey = Qt::Key_W;
    int moveDownKey = Qt::Key_S;
    int moveLeftKey = Qt::Key_A;
    int moveRightKey = Qt::Key_D;
    int undoKey = Qt::Key_Z;
    int saveKey = Qt::Key_C;
    int restartKey = Qt::Key_R;
    int menuKey = Qt::Key_B;
};
