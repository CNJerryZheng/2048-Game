#include "mainwindow.h"
#include "boardwidget.h"
#include "theme.h"
#include "widget_helpers.h"
#include "mod_loader.h"

extern "C"
{
#include "app_log.h"
#include "auth.h"
#include "config.h"
#include "game_mode.h"
#include "history.h"
#include "rank.h"
#include "storage.h"
#include "utils.h"
}

#include <QByteArray>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QCheckBox>
#include <QCryptographicHash>
#include <QDate>
#include <QDateTime>
#include <QDesktopServices>
#include <QColor>
#include <QDir>
#include <QFormLayout>
#include <QFile>
#include <QFont>
#include <QGridLayout>
#include <QHeaderView>
#include <QInputDialog>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QKeyEvent>
#include <QKeySequenceEdit>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QTextEdit>
#include <QTimer>
#include <QVBoxLayout>
#include <QVector>
#include <QUrl>
#include <QFileInfo>
#include <algorithm>

using WidgetHelpers::formatTime;
using WidgetHelpers::makeButton;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QDir().mkpath(QString::fromUtf8(GAME_DATA_DIRECTORY));
    QDir().mkpath(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/DLC");
    QDir().mkpath(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/Mods");
    QDir().mkpath(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/System");
    app_log_initialize(LOG_DATA_FILE);
    const ModLoader::LoadResult modResult = ModLoader::loadModeMods(modDirectoryPath());
    for (const QString &error : modResult.errors)
        APP_LOG_WARNING_MSG("mod", "%s", error.toUtf8().constData());
    APP_LOG_INFO_MSG("app", "application started");
    setWindowTitle("2048 Game");
    setMinimumSize(620, 720);

    pages = new QStackedWidget(this);
    setCentralWidget(pages);
    buildAuthPage();
    buildMenuPage();
    buildGamePage();
    buildDetailPage();
    loadKeyBindings();
    qApp->installEventFilter(this);
    pages->setCurrentWidget(authPage);

    gameTimer = new QTimer(this);
    gameTimer->setInterval(1000);
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::updateTimeLabel);

    setStyleSheet(Theme::applicationStyleSheet());
}

void MainWindow::buildAuthPage()
{
    authPage = new QWidget;
    auto *outer = new QVBoxLayout(authPage);
    auto *title = new QLabel("2048");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:64px;font-weight:800;color:#776e65;");
    outer->addStretch();
    outer->addWidget(title);

    authTabs = new QTabWidget;
    auto *loginTab = new QWidget;
    auto *loginLayout = new QFormLayout(loginTab);
    loginUsername = new QLineEdit;
    loginPassword = new QLineEdit;
    loginUsername->setMinimumHeight(46);
    loginPassword->setMinimumHeight(46);
    loginUsername->setPlaceholderText("请输入2-31位字母、数字或下划线");
    loginPassword->setPlaceholderText("请输入6-20位密码");
    loginPassword->setEchoMode(QLineEdit::Password);
    auto *showLoginPassword = new QCheckBox("显示密码");
    connect(showLoginPassword, &QCheckBox::toggled, loginPassword,
            [this](bool visible)
            {
                loginPassword->setEchoMode(visible ? QLineEdit::Normal
                                                   : QLineEdit::Password);
            });
    loginMessage = new QLabel;
    loginMessage->setWordWrap(true);
    loginMessage->setProperty("role", "hint");
    auto *loginButton = makeButton("登录");
    auto *guestButton = makeButton("游客进入（成绩不保存）");
    loginLayout->addRow("用户名", loginUsername);
    loginLayout->addRow("密码", loginPassword);
    loginLayout->addRow(QString(), showLoginPassword);
    loginLayout->addRow(loginMessage);
    loginLayout->addRow(loginButton);
    loginLayout->addRow(guestButton);
    authTabs->addTab(loginTab, "登录");

    auto *registerTab = new QWidget;
    auto *registerLayout = new QFormLayout(registerTab);
    registerUsername = new QLineEdit;
    registerPassword = new QLineEdit;
    registerConfirmation = new QLineEdit;
    for (QLineEdit *edit : {registerUsername, registerPassword, registerConfirmation})
        edit->setMinimumHeight(46);
    registerUsername->setPlaceholderText("请输入2-31位字母、数字或下划线");
    registerPassword->setPlaceholderText("请输入6-20位密码");
    registerConfirmation->setPlaceholderText("请再次输入密码");
    registerPassword->setEchoMode(QLineEdit::Password);
    registerConfirmation->setEchoMode(QLineEdit::Password);
    registerMessage = new QLabel;
    registerMessage->setWordWrap(true);
    registerMessage->setProperty("role", "hint");
    auto *registerButton = makeButton("注册");
    registerLayout->addRow("用户名", registerUsername);
    registerLayout->addRow("密码", registerPassword);
    registerLayout->addRow("确认密码", registerConfirmation);
    registerLayout->addRow(registerMessage);
    registerLayout->addRow(registerButton);
    authTabs->addTab(registerTab, "注册");

    outer->addWidget(authTabs);
    outer->addStretch();
    pages->addWidget(authPage);

    connect(loginButton, &QPushButton::clicked, this, &MainWindow::login);
    connect(loginPassword, &QLineEdit::returnPressed, this, &MainWindow::login);
    connect(guestButton, &QPushButton::clicked, this, &MainWindow::enterAsGuest);
    connect(registerButton, &QPushButton::clicked, this, &MainWindow::registerUser);
}

void MainWindow::buildMenuPage()
{
    menuPage = new QWidget;
    auto *layout = new QVBoxLayout(menuPage);
    layout->setContentsMargins(90, 55, 90, 55);
    auto *title = new QLabel("2048");
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:64px;font-weight:800;color:#776e65;");
    welcomeLabel = new QLabel;
    welcomeLabel->setAlignment(Qt::AlignCenter);
    welcomeLabel->setStyleSheet("font-size:20px;font-weight:600;");
    auto *newGameButton = makeButton("开始新游戏");
    continueButton = makeButton("继续上次游戏");
    auto *rankingButton = makeButton("排行榜");
    historyButton = makeButton("历史成绩");
    userCenterButton = makeButton("用户中心");
    auto *settingsButton = makeButton("设置");
    auto *helpButton = makeButton("帮助");
    auto *logoutButton = makeButton("退出登录");
    modeButton = makeButton("经典模式");
    for (QPushButton *button : {newGameButton, continueButton, rankingButton,
                                historyButton, userCenterButton, settingsButton,
                                helpButton, logoutButton, modeButton})
        button->setStyleSheet("font-size:18px;font-weight:800;");

    layout->addWidget(title);
    layout->addWidget(welcomeLabel);
    layout->addSpacing(20);
    layout->addWidget(newGameButton);
    layout->addWidget(continueButton);
    layout->addWidget(rankingButton);
    layout->addWidget(historyButton);
    layout->addWidget(userCenterButton);
    layout->addWidget(settingsButton);
    layout->addWidget(helpButton);
    layout->addWidget(logoutButton);
    layout->addStretch();
    layout->addWidget(modeButton, 0, Qt::AlignLeft);
    pages->addWidget(menuPage);

    connect(newGameButton, &QPushButton::clicked, this, &MainWindow::beginNewGame);
    connect(continueButton, &QPushButton::clicked, this, &MainWindow::startSavedGame);
    connect(rankingButton, &QPushButton::clicked, this, &MainWindow::showRanking);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showHistory);
    connect(userCenterButton, &QPushButton::clicked, this, &MainWindow::showUserCenter);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
    connect(modeButton, &QPushButton::clicked, this, &MainWindow::cycleGameMode);
}

void MainWindow::buildGamePage()
{
    gamePage = new QWidget;
    auto *outer = new QVBoxLayout(gamePage);
    outer->setContentsMargins(45, 25, 45, 25);
    auto *header = new QGridLayout;
    auto *title = new QLabel("2048");
    title->setStyleSheet("font-size:44px;font-weight:800;color:#776e65;");
    scoreLabel = new QLabel;
    stepLabel = new QLabel;
    timeLabel = new QLabel;
    modeLabel = new QLabel;
    for (QLabel *label : {scoreLabel, stepLabel, timeLabel, modeLabel})
        label->setStyleSheet("font-size:16px;font-weight:700;");
    header->addWidget(title, 0, 0, 2, 1);
    header->addWidget(scoreLabel, 0, 1);
    header->addWidget(stepLabel, 1, 1);
    header->addWidget(timeLabel, 0, 2);
    header->addWidget(modeLabel, 1, 2);
    outer->addLayout(header);

    boardWidget = new BoardWidget;
    outer->addWidget(boardWidget, 1);
    gameMessage = new QLabel("方向键或 W/A/S/D 移动方块");
    gameMessage->setAlignment(Qt::AlignCenter);
    gameMessage->setProperty("role", "hint");
    outer->addWidget(gameMessage);

    auto *buttons = new QGridLayout;
    undoButton = makeButton("撤销一步");
    auto *restartButton = makeButton("重新开始");
    saveButton = makeButton("保存游戏");
    auto *menuButton = makeButton("返回主菜单");
    buttons->addWidget(undoButton, 0, 0);
    buttons->addWidget(restartButton, 0, 1);
    buttons->addWidget(saveButton, 1, 0);
    buttons->addWidget(menuButton, 1, 1);
    outer->addLayout(buttons);
    pages->addWidget(gamePage);

    connect(undoButton, &QPushButton::clicked, this, &MainWindow::undoMove);
    connect(restartButton, &QPushButton::clicked, this, &MainWindow::restartGame);
    connect(saveButton, &QPushButton::clicked, this, [this]
            { saveGame(); });
    connect(menuButton, &QPushButton::clicked, this, &MainWindow::returnToMenu);
    connect(boardWidget, &BoardWidget::animationFinished,
            this, &MainWindow::finishAnimatedMove);
}

void MainWindow::buildDetailPage()
{
    detailPage = new QWidget;
    detailLayout = new QVBoxLayout(detailPage);
    detailLayout->setContentsMargins(45, 30, 45, 30);
    pages->addWidget(detailPage);
}

void MainWindow::showDetailPage(const QString &titleText, QWidget *content,
                                void (MainWindow::*backAction)())
{
    while (QLayoutItem *item = detailLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    auto *backButton = makeButton(backAction == nullptr ? "返回主菜单" : "返回");
    backButton->setMaximumWidth(150);
    auto *title = new QLabel(titleText);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:30px;font-weight:800;color:#776e65;");
    detailLayout->addWidget(backButton, 0, Qt::AlignLeft);
    detailLayout->addWidget(title);
    detailLayout->addWidget(content, 1);
    if (backAction == nullptr)
        connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    else
        connect(backButton, &QPushButton::clicked, this, backAction);
    pages->setCurrentWidget(detailPage);
}

void MainWindow::login()
{
    const QByteArray username = loginUsername->text().trimmed().toUtf8();
    clearSessionPassword();
    const QByteArray password = loginPassword->text().toUtf8();
    if (username == "Administrator" && password == "2048Game")
    {
        clearSessionPassword();
        currentUser = "Administrator";
        guestMode = false;
        adminMode = true;
        hasUnsavedChanges = false;
        loginPassword->clear();
        showAdminDashboard();
        return;
    }
    const AuthResult result = auth_login_user(USER_DATA_FILE,
                                              username.constData(),
                                              password.constData());
    if (result != AUTH_OK)
    {
        APP_LOG_WARNING_MSG("auth", "login failed for user=%s result=%d",
                            username.constData(), (int)result);
        loginMessage->setText(authMessage(result));
        return;
    }
    currentUser = QString::fromUtf8(username);
    sessionPassword = password;
    APP_LOG_INFO_MSG("auth", "login succeeded for user=%s", username.constData());
    if (accountStatus(currentUser) == "deleted")
    {
        loginMessage->setText("账号已注销，无法登录。");
        currentUser.clear();
        clearSessionPassword();
        loginPassword->clear();
        return;
    }
    ensureUserUid(currentUser);
    guestMode = false;
    adminMode = isConfiguredAdmin(currentUser);
    hasUnsavedChanges = false;
    migrateLegacyUserData();
    loadKeyBindings();
    loginPassword->clear();
    loginMessage->clear();
    showMenu();
}

void MainWindow::registerUser()
{
    if (registerPassword->text() != registerConfirmation->text())
    {
        registerMessage->setText("两次输入的密码不一致。");
        return;
    }
    const QByteArray username = registerUsername->text().trimmed().toUtf8();
    const QByteArray password = registerPassword->text().toUtf8();
    const AuthResult result = auth_register_user(USER_DATA_FILE,
                                                 username.constData(),
                                                 password.constData());
    registerMessage->setText(result == AUTH_OK ? "注册成功，请切换到登录页。"
                                               : authMessage(result));
    if (result == AUTH_OK)
    {
        if (accountStatus(QString::fromUtf8(username)) == "deleted")
        {
            QSettings accounts(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/accounts.ini",
                               QSettings::IniFormat);
            accounts.remove("users/" + QString::fromUtf8(username));
            accounts.remove("status/" + QString::fromUtf8(username));
            accounts.sync();
        }
        ensureUserUid(QString::fromUtf8(username));
        setAccountStatus(QString::fromUtf8(username), "normal");
        (void)rank_delete_user(SCORES_DATA_FILE, username.constData());
        APP_LOG_INFO_MSG("auth", "user registered: %s", username.constData());
        loginUsername->setText(registerUsername->text().trimmed());
        registerPassword->clear();
        registerConfirmation->clear();
        authTabs->setCurrentIndex(0);
    }
}

void MainWindow::enterAsGuest()
{
    clearSessionPassword();
    APP_LOG_INFO_MSG("auth", "guest session started");
    currentUser = "游客";
    guestMode = true;
    hasUnsavedChanges = false;
    loadKeyBindings();
    showMenu();
}

void MainWindow::showMenu()
{
    gameActive = false;
    hasUnsavedChanges = false;
    hasQueuedMove = false;
    if (boardWidget != nullptr)
        boardWidget->cancelAnimation();
    gameTimer->stop();
    sessionTimer.invalidate();
    welcomeLabel->setText(guestMode ? "游客模式 · 成绩与存档不会保存"
                                    : QString("欢迎，%1").arg(displayNameForUser(currentUser)));
    const QByteArray username = currentUser.toUtf8();
    continueButton->setEnabled(!guestMode && storage_save_exists(
                                                 saveFilePath().toUtf8().constData(), username.constData()));
    userCenterButton->setEnabled(!guestMode);
    historyButton->setEnabled(!guestMode);
    updateModeButton();
    pages->setCurrentWidget(menuPage);
}

void MainWindow::beginNewGame()
{
    if (isCurrentUserBanned())
    {
        QMessageBox::warning(this, "账号已封禁",
                             "当前账号已被封禁，不能进入游戏。排行榜、历史记录和个人主页仍可查看。");
        return;
    }
    const QByteArray username = currentUser.toUtf8();
    if (!guestMode && storage_save_exists(saveFilePath().toUtf8().constData(), username.constData()))
    {
        const auto answer = QMessageBox::question(
            this, "开始新游戏", "开始新游戏会覆盖当前存档，是否继续？");
        if (answer != QMessageBox::Yes)
            return;
        (void)storage_delete_save(saveFilePath().toUtf8().constData(), username.constData());
    }
    const QByteArray mode = selectedModeId().toUtf8();
    (void)game_mode_start(mode.constData(), &board);
    APP_LOG_INFO_MSG("game", "new game started user=%s guest=%d mode=%s",
                     currentUser.toUtf8().constData(), guestMode ? 1 : 0, board.mode);
    canUndo = false;
    hasQueuedMove = false;
    gameActive = true;
    hasUnsavedChanges = !guestMode;
    sessionTimer.start();
    gameTimer->start();
    pages->setCurrentWidget(gamePage);
    renderBoard();
}

void MainWindow::startSavedGame()
{
    if (guestMode)
        return;
    if (isCurrentUserBanned())
    {
        QMessageBox::warning(this, "账号已封禁",
                             "当前账号已被封禁，不能进入游戏。排行榜、历史记录和个人主页仍可查看。");
        return;
    }
    const QByteArray username = currentUser.toUtf8();
    if (!storage_load_save(saveFilePath().toUtf8().constData(), username.constData(), &board))
    {
        QMessageBox::warning(this, "读取失败", "没有找到有效存档。");
        showMenu();
        return;
    }
    board.game_over = false;
    APP_LOG_INFO_MSG("game", "save loaded user=%s mode=%s step=%d",
                     username.constData(), board.mode, board.step);
    canUndo = false;
    hasQueuedMove = false;
    gameActive = true;
    hasUnsavedChanges = false;
    sessionTimer.start();
    gameTimer->start();
    pages->setCurrentWidget(gamePage);
    renderBoard();
}

void MainWindow::saveGame(bool showMessage)
{
    if (isCurrentUserBanned())
        return;
    if (guestMode)
    {
        if (showMessage)
            gameMessage->setText("游客模式不保存游戏。");
        return;
    }
    board.elapsed_seconds = currentElapsedSeconds();
    sessionTimer.restart();
    const QByteArray username = currentUser.toUtf8();
    const bool success = storage_save_game(saveFilePath().toUtf8().constData(),
                                           username.constData(), &board);
    app_log_write(success ? APP_LOG_INFO : APP_LOG_ERROR, "storage",
                  "save user=%s step=%d success=%d",
                  username.constData(), board.step, success ? 1 : 0);
    if (showMessage)
        gameMessage->setText(success ? "游戏已加密保存。"
                                     : "保存失败，请检查数据目录。");
    if (success)
        hasUnsavedChanges = false;
}

void MainWindow::returnToMenu()
{
    if (!guestMode && !hasUnsavedChanges)
    {
        showMenu();
        return;
    }
    QMessageBox box(QMessageBox::Question, "返回主菜单",
                    guestMode ? "返回主菜单将放弃当前游客棋局。"
                              : "返回主菜单前是否保存当前游戏？",
                    QMessageBox::NoButton, this);
    QPushButton *saveChoice = guestMode
                                  ? nullptr
                                  : box.addButton("保存并返回", QMessageBox::AcceptRole);
    auto *discardChoice = box.addButton("不保存并返回", QMessageBox::DestructiveRole);
    box.addButton("取消", QMessageBox::RejectRole);
    box.exec();
    if (box.clickedButton() == nullptr || box.buttonRole(box.clickedButton()) == QMessageBox::RejectRole)
        return;
    hasQueuedMove = false;
    boardWidget->cancelAnimation();
    if (saveChoice != nullptr && box.clickedButton() == saveChoice)
        saveGame(false);
    Q_UNUSED(discardChoice);
    showMenu();
}

void MainWindow::logout()
{
    if (gameActive && !guestMode && hasUnsavedChanges)
        saveGame(false);
    currentUser.clear();
    clearSessionPassword();
    guestMode = false;
    adminMode = false;
    gameActive = false;
    hasUnsavedChanges = false;
    gameTimer->stop();
    pages->setCurrentWidget(authPage);
}

void MainWindow::clearSessionPassword()
{
    if (!sessionPassword.isEmpty())
    {
        sessionPassword.fill('\0');
        sessionPassword.clear();
        sessionPassword.squeeze();
    }
}

void MainWindow::undoMove()
{
    if (!canUndo)
    {
        gameMessage->setText("当前没有可以撤销的操作。");
        return;
    }
    hasQueuedMove = false;
    boardWidget->cancelAnimation();
    board = undoBoard;
    canUndo = false;
    hasUnsavedChanges = !guestMode;
    sessionTimer.restart();
    renderBoard();
    gameMessage->setText("已撤销上一步。");
}

void MainWindow::restartGame()
{
    if (isCurrentUserBanned())
    {
        QMessageBox::warning(this, "账号已封禁",
                             "当前账号已被封禁，不能进入游戏。排行榜、历史记录和个人主页仍可查看。");
        return;
    }
    if (QMessageBox::question(this, "重新开始",
                              "确定放弃当前进度并重新开始吗？") != QMessageBox::Yes)
        return;
    hasQueuedMove = false;
    boardWidget->cancelAnimation();
    if (!guestMode)
    {
        const QByteArray username = currentUser.toUtf8();
        (void)storage_delete_save(saveFilePath().toUtf8().constData(), username.constData());
    }
    const QByteArray mode = selectedModeId().toUtf8();
    (void)game_mode_start(mode.constData(), &board);
    APP_LOG_INFO_MSG("game", "game restarted user=%s", currentUser.toUtf8().constData());
    canUndo = false;
    hasUnsavedChanges = !guestMode;
    sessionTimer.restart();
    renderBoard();
}

void MainWindow::showRanking()
{
    RankEntry entries[RANK_ENTRIES_MAX];
    const int count = rank_load_scores(SCORES_DATA_FILE,
                                       entries, RANK_ENTRIES_MAX);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);

    auto configureTable = [](QTableWidget *table)
    {
        table->setAlternatingRowColors(true);
        table->verticalHeader()->setVisible(false);
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->setEditTriggers(QAbstractItemView::NoEditTriggers);
        table->setSelectionMode(QAbstractItemView::SingleSelection);
        table->setSelectionBehavior(QAbstractItemView::SelectRows);
        table->setCursor(Qt::PointingHandCursor);
    };

    auto displayUsername = [this](const RankEntry &entry)
    {
        const QString username = QString::fromUtf8(entry.username);
        return entry.deleted ? QStringLiteral("（已注销）") + username
                             : displayNameForUser(username);
    };

    auto makeModeTable = [&](const QString &modeId)
    {
        int filteredCount = 0;
        for (int index = 0; index < count; ++index)
            if (modeId == QString::fromUtf8(entries[index].mode))
                ++filteredCount;
        const int visibleCount = qMin(filteredCount, RANK_TOP_COUNT);
        auto *page = new QWidget;
        auto *pageLayout = new QVBoxLayout(page);
        auto *table = new QTableWidget(visibleCount, 6);
        configureTable(table);
        table->setHorizontalHeaderLabels(
            {"排名", "用户名", "分数", "最大方块", "步数", "用时"});

        QVector<QString> profileUsers;
        int shownRow = 0;
        int modeRank = 0;
        int currentRank = -1;
        int currentScore = 0;
        for (int index = 0; index < count; ++index)
        {
            if (modeId != QString::fromUtf8(entries[index].mode))
                continue;
            ++modeRank;
            if (!guestMode && currentUser == QString::fromUtf8(entries[index].username))
            {
                currentRank = modeRank;
                currentScore = entries[index].score;
            }
            if (shownRow >= visibleCount)
                continue;
            table->setItem(shownRow, 0, new QTableWidgetItem(QString::number(modeRank)));
            const QString username = QString::fromUtf8(entries[index].username);
            profileUsers.append(username);
            table->setItem(shownRow, 1, new QTableWidgetItem(displayUsername(entries[index])));
            table->setItem(shownRow, 2, new QTableWidgetItem(QString::number(entries[index].score)));
            table->setItem(shownRow, 3, new QTableWidgetItem(QString::number(entries[index].max_tile)));
            table->setItem(shownRow, 4, new QTableWidgetItem(QString::number(entries[index].steps)));
            table->setItem(shownRow, 5, new QTableWidgetItem(formatTime(entries[index].elapsed_seconds)));
            if (!guestMode && currentUser == username)
            {
                for (int column = 0; column < table->columnCount(); ++column)
                {
                    table->item(shownRow, column)->setBackground(Theme::currentUserHighlight());
                    QFont font = table->item(shownRow, column)->font();
                    font.setBold(true);
                    table->item(shownRow, column)->setFont(font);
                }
            }
            ++shownRow;
        }
        connect(table, &QTableWidget::cellClicked, page,
                [this, profileUsers](int row, int)
                {
                    if (row >= 0 && row < profileUsers.size())
                    {
                        const QString username = profileUsers[row];
                        QTimer::singleShot(0, this, [this, username]
                                           { showPublicProfile(username); });
                    }
                });
        pageLayout->addWidget(table);
        if (!guestMode && currentRank > RANK_TOP_COUNT && visibleCount >= RANK_TOP_COUNT)
        {
            const int tenthScore = table->item(RANK_TOP_COUNT - 1, 2)->text().toInt();
            const int difference = qMax(0, tenthScore - currentScore + 1);
            auto *tip = new QLabel(QString("当前第 %1 名，离第十名还差 %2 分。")
                                       .arg(currentRank)
                                       .arg(difference));
            tip->setAlignment(Qt::AlignCenter);
            tip->setProperty("role", "hint");
            pageLayout->addWidget(tip);
        }
        else if (!guestMode && currentRank < 0)
        {
            auto *tip = new QLabel("当前分数过低，请继续游戏。");
            tip->setAlignment(Qt::AlignCenter);
            tip->setProperty("role", "hint");
            pageLayout->addWidget(tip);
        }
        return page;
    };

    auto makeDragonTable = [&]()
    {
        struct Period
        {
            QString name;
            qint64 since;
        };
        const QDateTime now = QDateTime::currentDateTime();
        const QList<Period> periods = {
            {"今日", QDateTime(now.date().startOfDay()).toSecsSinceEpoch()},
            {"本周", now.addDays(-7).toSecsSinceEpoch()},
            {"本月", QDateTime(QDate(now.date().year(), now.date().month(), 1).startOfDay()).toSecsSinceEpoch()}};
        auto *page = new QWidget;
        auto *pageLayout = new QVBoxLayout(page);
        for (int modeIndex = 0; modeIndex < game_mode_count(); ++modeIndex)
        {
            const GameModeDefinition *mode = game_mode_at(modeIndex);
            if (mode == nullptr)
                continue;
            const QString modeId = QString::fromUtf8(mode->id);
            auto *title = new QLabel(QString("%1").arg(modeDisplayName(mode->id)));
            title->setAlignment(Qt::AlignCenter);
            title->setStyleSheet("background:#eee8df;color:#3c3a32;border-radius:6px;padding:6px;font-weight:800;");
            pageLayout->addWidget(title);
            auto *table = new QTableWidget(periods.size(), 6);
            configureTable(table);
            table->setHorizontalHeaderLabels(
                {"榜单", "用户名", "分数", "最大方块", "步数", "用时"});
            QVector<QString> profileUsers;
            for (int periodIndex = 0; periodIndex < periods.size(); ++periodIndex)
            {
                const Period &period = periods[periodIndex];
                int winner = -1;
                for (int index = 0; index < count; ++index)
                {
                    if (modeId != QString::fromUtf8(entries[index].mode))
                        continue;
                    if (entries[index].achieved_at > 0 && entries[index].achieved_at < period.since)
                        continue;
                    winner = index;
                    break;
                }
                table->setItem(periodIndex, 0, new QTableWidgetItem(period.name));
                if (winner < 0)
                {
                    table->setItem(periodIndex, 1, new QTableWidgetItem("暂无"));
                    table->setItem(periodIndex, 2, new QTableWidgetItem("-"));
                    table->setItem(periodIndex, 3, new QTableWidgetItem("-"));
                    table->setItem(periodIndex, 4, new QTableWidgetItem("-"));
                    table->setItem(periodIndex, 5, new QTableWidgetItem("-"));
                    profileUsers.append(QString());
                    continue;
                }
                profileUsers.append(QString::fromUtf8(entries[winner].username));
                table->setItem(periodIndex, 1, new QTableWidgetItem(displayUsername(entries[winner])));
                table->setItem(periodIndex, 2, new QTableWidgetItem(QString::number(entries[winner].score)));
                table->setItem(periodIndex, 3, new QTableWidgetItem(QString::number(entries[winner].max_tile)));
                table->setItem(periodIndex, 4, new QTableWidgetItem(QString::number(entries[winner].steps)));
                table->setItem(periodIndex, 5, new QTableWidgetItem(formatTime(entries[winner].elapsed_seconds)));
            }
            connect(table, &QTableWidget::cellClicked, table,
                    [this, profileUsers](int row, int)
                    {
                        if (row >= 0 && row < profileUsers.size() && !profileUsers[row].isEmpty())
                        {
                            const QString username = profileUsers[row];
                            QTimer::singleShot(0, this, [this, username]
                                               { showPublicProfile(username); });
                        }
                    });
            pageLayout->addWidget(table);
        }
        pageLayout->addStretch();
        return page;
    };

    auto *tabs = new QTabWidget;
    tabs->addTab(makeDragonTable(), "全区卧龙凤雏");
    const QStringList modes = enabledModeIds();
    for (const QString &modeId : modes)
        tabs->addTab(makeModeTable(modeId), modeDisplayName(modeId.toUtf8().constData()));
    layout->addWidget(tabs);
    showDetailPage("排行榜", content);
}

void MainWindow::showPublicProfile(const QString &username)
{
    RankEntry entries[RANK_ENTRIES_MAX];
    const int count = rank_load_scores(SCORES_DATA_FILE, entries, RANK_ENTRIES_MAX);
    int rank = -1;
    RankEntry record = {};
    for (int index = 0; index < count; ++index)
    {
        if (username == QString::fromUtf8(entries[index].username))
        {
            rank = index + 1;
            record = entries[index];
            break;
        }
    }
    if (rank < 0)
        return;
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout;
    const QString shownName = record.deleted ? QStringLiteral("（已注销）") + username
                                             : displayNameForUser(username);
    form->addRow("用户名", new QLabel(shownName));
    form->addRow("用户 UID", new QLabel(record.deleted ? "账户异常，无法显示"
                                                       : userUid(username)));
    form->addRow("历史排名", new QLabel(QString("第 %1 名").arg(rank)));
    form->addRow("历史最高分", new QLabel(QString::number(record.score)));
    form->addRow("历史最大方块", new QLabel(QString::number(record.max_tile)));
    form->addRow("历史步数 / 用时",
                 new QLabel(QString("%1 步 / %2").arg(record.steps).arg(formatTime(record.elapsed_seconds))));
    if (record.deleted)
    {
        form->addRow("邮箱地址", new QLabel("账户异常，无法显示"));
        form->addRow("个人简介", new QLabel("账户异常，无法显示"));
        form->addRow("账户状态", new QLabel("账户异常"));
    }
    else
    {
        QSettings profile(userDirectoryForUsername(username) + "/profile.ini",
                          QSettings::IniFormat);
        form->addRow("邮箱地址", new QLabel(profile.value("email", "未设置").toString()));
        auto *biography = new QLabel(profile.value(
                                                "biography", "这个人很懒，什么都没有留下")
                                         .toString());
        biography->setWordWrap(true);
        form->addRow("个人简介", biography);
        form->addRow("账户状态", new QLabel(accountStatusText(username, false)));
    }
    layout->addLayout(form);
    layout->addStretch();
    showDetailPage("用户主页", content, &MainWindow::showRanking);
}

void MainWindow::showHistory()
{
    if (guestMode)
        return;
    HistoryEntry entries[HISTORY_ENTRIES_MAX];
    const QByteArray username = currentUser.toUtf8();
    const int count = history_load_user(historyFilePath().toUtf8().constData(), username.constData(),
                                        entries, HISTORY_ENTRIES_MAX);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *table = new QTableWidget(count, 8);
    table->setAlternatingRowColors(true);
    table->verticalHeader()->setVisible(false);
    table->setHorizontalHeaderLabels(
        {"完成日期", "具体时间", "分数", "最大方块", "步数", "用时", "模式", "用户"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int row = 0; row < count; ++row)
    {
        const HistoryEntry &entry = entries[count - row - 1];
        const QDateTime finished = QDateTime::fromSecsSinceEpoch(entry.finished_at);
        table->setItem(row, 0, new QTableWidgetItem(finished.toString("yyyy-MM-dd")));
        table->setItem(row, 1, new QTableWidgetItem(finished.toString("HH:mm:ss")));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(entry.score)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(entry.max_tile)));
        table->setItem(row, 4, new QTableWidgetItem(QString::number(entry.steps)));
        table->setItem(row, 5, new QTableWidgetItem(formatTime(entry.elapsed_seconds)));
        table->setItem(row, 6, new QTableWidgetItem(modeDisplayName(entry.mode)));
        table->setItem(row, 7, new QTableWidgetItem(displayNameForUser(QString::fromUtf8(entry.username))));
    }
    layout->addWidget(table);
    showDetailPage("我的历史成绩", content);
}

void MainWindow::showUserCenter()
{
    if (guestMode)
        return;
    QSettings profile(profileFilePath(), QSettings::IniFormat);
    const QString email = profile.value("email", "未设置").toString();
    const QString biography = profile.value(
                                         "biography", "这个人很懒，什么都没有留下")
                                  .toString();
    RankEntry entries[RANK_ENTRIES_MAX];
    const int count = rank_load_scores(SCORES_DATA_FILE, entries, RANK_ENTRIES_MAX);
    int rank = -1;
    RankEntry best = {};
    for (int index = 0; index < count; ++index)
    {
        if (currentUser == QString::fromUtf8(entries[index].username))
        {
            rank = index + 1;
            best = entries[index];
            break;
        }
    }
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *card = new QWidget;
    card->setObjectName("profileCard");
    card->setStyleSheet("QWidget#profileCard{background:#fffdf8;border:1px solid #ded6cc;border-radius:10px;padding:12px;}"
                        "QWidget#profileCard QLabel{border:none;background:transparent;padding:0;}");
    auto *form = new QFormLayout(card);
    form->addRow("用户名", new QLabel(displayNameForUser(currentUser)));
    form->addRow("用户 UID", new QLabel(userUid(currentUser)));
    form->addRow("邮箱地址", new QLabel(email));
    form->addRow("账户状态", new QLabel(accountStatusText(currentUser, false)));
    form->addRow("排行榜名次", new QLabel(rank > 0 ? QString("第 %1 名").arg(rank) : "暂无排名"));
    form->addRow("最高分数", new QLabel(rank > 0 ? QString::number(best.score) : "暂无"));
    form->addRow("最大方块", new QLabel(rank > 0 ? QString::number(best.max_tile) : "暂无"));
    form->addRow("最佳局步数 / 用时",
                 new QLabel(rank > 0 ? QString("%1 步 / %2").arg(best.steps).arg(formatTime(best.elapsed_seconds)) : "暂无"));
    auto *bio = new QLabel(biography);
    bio->setWordWrap(true);
    form->addRow("个人简介", bio);
    layout->addWidget(card);
    auto *editButton = makeButton("编辑个人资料");
    auto *accountButton = makeButton("账户设置");
    for (QPushButton *button : {editButton, accountButton})
        button->setStyleSheet("font-size:18px;font-weight:800;");
    layout->addWidget(editButton);
    layout->addWidget(accountButton);
    layout->addStretch();
    connect(editButton, &QPushButton::clicked, this, &MainWindow::showEditProfile);
    connect(accountButton, &QPushButton::clicked, this, &MainWindow::showAccountSettings);
    showDetailPage("用户中心", content);
}

void MainWindow::showEditProfile()
{
    QSettings profile(profileFilePath(), QSettings::IniFormat);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout;
    auto *username = new QLineEdit(currentUser);
    auto *email = new QLineEdit(profile.value("email").toString());
    auto *biography = new QTextEdit(profile.value(
                                               "biography", "这个人很懒，什么都没有留下")
                                        .toString());
    biography->setMaximumHeight(120);
    form->addRow("用户名", username);
    form->addRow("邮箱地址", email);
    form->addRow("个人简介", biography);
    layout->addLayout(form);
    auto *message = new QLabel;
    message->setProperty("role", "hint");
    message->setWordWrap(true);
    auto *saveButton = makeButton("保存个人资料");
    layout->addWidget(message);
    layout->addWidget(saveButton);
    layout->addStretch();
    connect(saveButton, &QPushButton::clicked, content,
            [this, username, email, biography, message]
            {
                const QString newName = username->text().trimmed();
                if (newName.isEmpty())
                {
                    message->setText("用户名不能为空。");
                    return;
                }
                const QString oldName = currentUser;
                const QString oldDirectory = userDirectoryPath();
                QSettings oldProfile(profileFilePath(), QSettings::IniFormat);
                const qint64 lastRename = oldProfile.value("lastUsernameChange", 0).toLongLong();
                const qint64 now = QDateTime::currentSecsSinceEpoch();
                if (newName != oldName && !isConfiguredAdmin(oldName) && lastRename > 0 &&
                    now < lastRename + usernameCooldownSeconds())
                {
                    const qint64 remaining = lastRename + usernameCooldownSeconds() - now;
                    const qint64 remainingMinutes = (remaining + 59) / 60;
                    message->setText(QString("距上次更改用户名不足三天，请 %1 小时 %2 分钟后更改。")
                                         .arg(remainingMinutes / 60)
                                         .arg(remainingMinutes % 60));
                    return;
                }
                if (newName != oldName)
                {
                    if (QMessageBox::question(
                            this, "确认修改用户名",
                            "用户名更改后三天内无法再次更改，是否更改用户名？",
                            QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
                        return;
                    const QByteArray oldUser = oldName.toUtf8();
                    const QByteArray newUser = newName.toUtf8();
                    if (sessionPassword.isEmpty())
                    {
                        message->setText(u8"当前登录会话缺少密码缓存，请退出后重新登录再修改用户名。");
                        return;
                    }
                    const AuthResult result = auth_rename_user(
                        USER_DATA_FILE, oldUser.constData(), newUser.constData(), sessionPassword.constData());
                    if (result != AUTH_OK)
                    {
                        message->setText(authMessage(result));
                        return;
                    }
                    QSettings accounts(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/accounts.ini",
                                       QSettings::IniFormat);
                    const QString uid = accounts.value("users/" + oldName).toString();
                    const QString status = accounts.value("status/" + oldName, "normal").toString();
                    if (!uid.isEmpty())
                    {
                        accounts.remove("users/" + oldName);
                        accounts.remove("status/" + oldName);
                        accounts.setValue("users/" + newName, uid);
                        accounts.setValue("status/" + newName, status);
                        accounts.sync();
                    }
                    (void)rank_rename_user(SCORES_DATA_FILE, oldUser.constData(), newUser.constData());
                    (void)history_rename_user((oldDirectory + "/history.dat").toUtf8().constData(),
                                              oldUser.constData(), newUser.constData());
                    Board savedBoard = {};
                    const bool hasSave = storage_load_save((oldDirectory + "/save.dat").toUtf8().constData(),
                                                           oldUser.constData(), &savedBoard);
                    oldProfile.sync();
                    currentUser = newName;
                    if (hasSave)
                        (void)storage_save_game((oldDirectory + "/save.dat").toUtf8().constData(),
                                                newUser.constData(), &savedBoard);
                }
                QSettings updated(profileFilePath(), QSettings::IniFormat);
                updated.setValue("email", email->text().trimmed());
                updated.setValue("biography", biography->toPlainText().trimmed().isEmpty()
                                                  ? "这个人很懒，什么都没有留下"
                                                  : biography->toPlainText().trimmed());
                if (newName != oldName && !isConfiguredAdmin(newName))
                    updated.setValue("lastUsernameChange", QDateTime::currentSecsSinceEpoch());
                updated.sync();
                loadKeyBindings();
                QMessageBox::information(this, "个人资料", "个人资料已保存。");
                showUserCenter();
            });
    showDetailPage("编辑个人资料", content, &MainWindow::showUserCenter);
}

void MainWindow::showAccountSettings()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *passwordButton = makeButton("修改密码");
    auto *historyButton = makeButton("清理历史数据");
    auto *deleteButton = makeButton("注销账号");
    for (QPushButton *button : {passwordButton, historyButton, deleteButton})
        button->setStyleSheet("font-size:18px;font-weight:800;");
    layout->addWidget(passwordButton);
    layout->addWidget(historyButton);
    layout->addWidget(deleteButton);
    layout->addStretch();
    connect(passwordButton, &QPushButton::clicked, this, &MainWindow::showPasswordSettings);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showClearHistory);
    connect(deleteButton, &QPushButton::clicked, this, &MainWindow::showDeleteAccount);
    showDetailPage("账户设置", content, &MainWindow::showUserCenter);
}

void MainWindow::showPasswordSettings()
{
    if (guestMode)
        return;
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout;
    auto *oldPassword = new QLineEdit;
    auto *newPassword = new QLineEdit;
    auto *confirmation = new QLineEdit;
    for (QLineEdit *edit : {oldPassword, newPassword, confirmation})
        edit->setEchoMode(QLineEdit::Password);
    auto *showPasswords = new QCheckBox("显示密码");
    connect(showPasswords, &QCheckBox::toggled, content,
            [oldPassword, newPassword, confirmation](bool visible)
            {
                for (QLineEdit *edit : {oldPassword, newPassword, confirmation})
                    edit->setEchoMode(visible ? QLineEdit::Normal
                                              : QLineEdit::Password);
            });
    oldPassword->setPlaceholderText("请输入当前密码");
    newPassword->setPlaceholderText("请输入6-20位新密码");
    confirmation->setPlaceholderText("请再次输入新密码");
    form->addRow("当前用户", new QLabel(displayNameForUser(currentUser)));
    form->addRow("当前密码", oldPassword);
    form->addRow("新密码", newPassword);
    form->addRow("确认新密码", confirmation);
    layout->addLayout(form);
    layout->addWidget(showPasswords);
    auto *changeButton = makeButton("修改密码");
    auto *clearButton = makeButton("清理历史数据");
    auto *deleteButton = makeButton("注销账号");
    clearButton->hide();
    deleteButton->hide();
    auto *message = new QLabel;
    message->setWordWrap(true);
    message->setProperty("role", "hint");
    layout->addWidget(message);
    layout->addWidget(changeButton);
    layout->addWidget(clearButton);
    layout->addWidget(deleteButton);

    connect(changeButton, &QPushButton::clicked, content,
            [this, oldPassword, newPassword, confirmation, message]
            {
                if (newPassword->text() != confirmation->text())
                {
                    message->setText("两次输入的新密码不一致。");
                    return;
                }
                const QByteArray user = currentUser.toUtf8();
                const QByteArray oldValue = oldPassword->text().toUtf8();
                const QByteArray newValue = newPassword->text().toUtf8();
                const AuthResult result = auth_change_password(
                    USER_DATA_FILE, user.constData(), oldValue.constData(), newValue.constData());
                if (result == AUTH_OK)
                {
                    APP_LOG_INFO_MSG("auth", "password changed user=%s", user.constData());
                    QMessageBox::information(this, "修改密码",
                                             "密码修改成功，请重新登录。");
                    logout();
                    return;
                }
                message->setText(authMessage(result));
            });
    connect(clearButton, &QPushButton::clicked, content,
            [this, oldPassword, message]
            {
                if (oldPassword->text().isEmpty())
                {
                    message->setText("清理数据前请输入当前密码。");
                    return;
                }
                const QByteArray user = currentUser.toUtf8();
                const QByteArray password = oldPassword->text().toUtf8();
                const AuthResult authResult = auth_login_user(
                    USER_DATA_FILE, user.constData(), password.constData());
                if (authResult != AUTH_OK)
                {
                    message->setText(authMessage(authResult));
                    return;
                }
                if (QMessageBox::question(this, "清理历史数据",
                                          "将永久删除当前账号的全部历史成绩，是否继续？") != QMessageBox::Yes)
                    return;
                const bool historyOk = history_delete_user(historyFilePath().toUtf8().constData(), user.constData());
                APP_LOG_INFO_MSG("account", "history cleared user=%s history=%d",
                                 user.constData(), historyOk ? 1 : 0);
                message->setText(historyOk ? "历史成绩已清理。"
                                           : "历史成绩清理失败，请查看日志。");
                oldPassword->clear();
            });
    connect(deleteButton, &QPushButton::clicked, content,
            [this, oldPassword, message]
            {
                if (oldPassword->text().isEmpty())
                {
                    message->setText("删除账号前请输入当前密码。");
                    return;
                }
                if (QMessageBox::warning(this, "注销账号",
                                         "这是不可恢复操作，账号及其全部数据将永久删除。是否继续？",
                                         QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
                    return;
                bool accepted = false;
                const QString confirmation = QInputDialog::getText(
                    this, "再次确认密码", "请再次输入当前密码：",
                    QLineEdit::Password, QString(), &accepted);
                if (!accepted)
                    return;
                if (confirmation != oldPassword->text())
                {
                    message->setText("两次输入的密码不一致，已取消注销。");
                    return;
                }
                const QByteArray user = currentUser.toUtf8();
                const QByteArray password = oldPassword->text().toUtf8();
                const AuthResult result = auth_delete_user(
                    USER_DATA_FILE, user.constData(), password.constData());
                if (result != AUTH_OK)
                {
                    message->setText(authMessage(result));
                    return;
                }
                const QString accountDirectory = userDirectoryPath();
                setAccountStatus(currentUser, "deleted");
                (void)rank_mark_deleted(SCORES_DATA_FILE, user.constData());
                (void)QDir(accountDirectory).removeRecursively();
                APP_LOG_INFO_MSG("account", "account deleted user=%s", user.constData());
                QMessageBox::information(this, "注销账号", "账号已永久注销。");
                logout();
            });
    showDetailPage("修改密码", content, &MainWindow::showAccountSettings);
}

void MainWindow::showClearHistory()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *password = new QLineEdit;
    password->setEchoMode(QLineEdit::Password);
    auto *showPassword = new QCheckBox("显示密码");
    connect(showPassword, &QCheckBox::toggled, password,
            [password](bool visible)
            { password->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password); });
    password->setPlaceholderText("请输入当前密码");
    auto *message = new QLabel;
    message->setProperty("role", "hint");
    auto *clearButton = makeButton("清理历史数据");
    layout->addWidget(new QLabel("验证密码后可永久清理当前账号的历史成绩。"));
    layout->addWidget(password);
    layout->addWidget(showPassword);
    layout->addWidget(message);
    layout->addWidget(clearButton);
    layout->addStretch();
    connect(clearButton, &QPushButton::clicked, content, [this, password, message]
            {
        const QByteArray user = currentUser.toUtf8();
        const QByteArray pass = password->text().toUtf8();
        const AuthResult result = auth_login_user(USER_DATA_FILE, user.constData(), pass.constData());
        if (result != AUTH_OK)
        {
            message->setText(authMessage(result));
            return;
        }
        if (QMessageBox::warning(this, "清理历史数据",
                                 "历史成绩删除后无法恢复，是否继续？",
                                 QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return;
        const bool success = history_delete_user(historyFilePath().toUtf8().constData(), user.constData());
        if (success)
        {
            QMessageBox::information(this, "清理历史数据", "历史成绩已清理。");
            showAccountSettings();
            return;
        }
        message->setText("清理失败，请查看日志。");
        password->clear(); });
    showDetailPage("清理历史数据", content, &MainWindow::showAccountSettings);
}

void MainWindow::showDeleteAccount()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *password = new QLineEdit;
    password->setEchoMode(QLineEdit::Password);
    auto *showPassword = new QCheckBox("显示密码");
    connect(showPassword, &QCheckBox::toggled, password,
            [password](bool visible)
            { password->setEchoMode(visible ? QLineEdit::Normal : QLineEdit::Password); });
    password->setPlaceholderText("请输入当前密码");
    auto *message = new QLabel;
    message->setProperty("role", "hint");
    auto *deleteButton = makeButton("注销账号");
    layout->addWidget(new QLabel("注销会永久删除账号及全部个人数据。"));
    layout->addWidget(password);
    layout->addWidget(showPassword);
    layout->addWidget(message);
    layout->addWidget(deleteButton);
    layout->addStretch();
    connect(deleteButton, &QPushButton::clicked, content, [this, password, message]
            {
        const QByteArray user = currentUser.toUtf8();
        const QByteArray firstPassword = password->text().toUtf8();
        const AuthResult verified = auth_login_user(USER_DATA_FILE, user.constData(), firstPassword.constData());
        if (verified != AUTH_OK)
        {
            message->setText(authMessage(verified));
            return;
        }
        if (QMessageBox::warning(this, "不可恢复操作",
                                 "注销账号是不可恢复操作，是否继续？",
                                 QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return;
        bool accepted = false;
        const QString secondPassword = QInputDialog::getText(
            this, "再次确认密码", "请再次输入当前密码：",
            QLineEdit::Password, QString(), &accepted);
        if (!accepted)
            return;
        if (secondPassword != password->text())
        {
            message->setText("两次输入的密码不一致，已取消注销。");
            return;
        }
        const QString directory = userDirectoryPath();
        const AuthResult result = auth_delete_user(
            USER_DATA_FILE, user.constData(), firstPassword.constData());
        if (result != AUTH_OK)
        {
            message->setText(authMessage(result));
            return;
        }
        setAccountStatus(currentUser, "deleted");
        (void)rank_mark_deleted(SCORES_DATA_FILE, user.constData());
        (void)QDir(directory).removeRecursively();
        QMessageBox::information(this, "注销账号", "账号已永久注销。");
        logout(); });
    showDetailPage("注销账号", content, &MainWindow::showAccountSettings);
}

void MainWindow::showSettings()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout;
    auto *interval = new QSpinBox;
    interval->setRange(0, 100);
    interval->setSpecialValueText("关闭自动保存");
    interval->setSuffix(" 步");
    interval->setValue(settings.value("autoSaveInterval", defaultAutoSaveInterval()).toInt());
    form->addRow("有效移动后自动保存", interval);
    auto makeKeyEdit = [](int key)
    {
        auto *edit = new QKeySequenceEdit(QKeySequence(key));
        edit->setMaximumSequenceLength(1);
        return edit;
    };
    auto *upEdit = makeKeyEdit(moveUpKey);
    auto *downEdit = makeKeyEdit(moveDownKey);
    auto *leftEdit = makeKeyEdit(moveLeftKey);
    auto *rightEdit = makeKeyEdit(moveRightKey);
    auto *undoEdit = makeKeyEdit(undoKey);
    auto *saveEdit = makeKeyEdit(saveKey);
    auto *restartEdit = makeKeyEdit(restartKey);
    auto *menuEdit = makeKeyEdit(menuKey);
    form->addRow("向上", upEdit);
    form->addRow("向下", downEdit);
    form->addRow("向左", leftEdit);
    form->addRow("向右", rightEdit);
    form->addRow("撤销一步", undoEdit);
    form->addRow("保存游戏", saveEdit);
    form->addRow("重新开始", restartEdit);
    form->addRow("返回主菜单", menuEdit);
    layout->addLayout(form);
    auto *modInfo = new QLabel(QString("DLC 目录：%1\n已注册模式数：%2")
                                   .arg(QDir(modDirectoryPath()).absolutePath())
                                   .arg(game_mode_count()));
    modInfo->setWordWrap(true);
    modInfo->setProperty("role", "hint");
    layout->addWidget(modInfo);
    auto *message = new QLabel;
    message->setProperty("role", "hint");
    auto *saveSettingsButton = makeButton("保存设置");
    auto *dlcManagerButton = makeButton("DLC 管理");
    auto *openModDirectoryButton = makeButton("打开 DLC 目录");
    QPushButton *adminSettingsButton = nullptr;
    if (canOpenAdminSettings())
        adminSettingsButton = makeButton("管理员设置");
    layout->addWidget(message);
    layout->addWidget(saveSettingsButton);
    layout->addWidget(dlcManagerButton);
    layout->addWidget(openModDirectoryButton);
    if (adminSettingsButton != nullptr)
        layout->addWidget(adminSettingsButton);
    layout->addStretch();
    connect(saveSettingsButton, &QPushButton::clicked, content,
            [this, interval, message, upEdit, downEdit, leftEdit, rightEdit,
             undoEdit, saveEdit, restartEdit, menuEdit]
            {
                const QList<QKeySequenceEdit *> edits = {
                    upEdit, downEdit, leftEdit, rightEdit,
                    undoEdit, saveEdit, restartEdit, menuEdit};
                QList<int> keys;
                for (QKeySequenceEdit *edit : edits)
                {
                    if (edit->keySequence().isEmpty())
                    {
                        message->setText("每项操作都必须设置一个按键。");
                        return;
                    }
                    const int key = edit->keySequence()[0].toCombined();
                    if (keys.contains(key))
                    {
                        message->setText("不同操作不能使用相同按键。");
                        return;
                    }
                    keys.append(key);
                }
                QSettings settings(settingsFilePath(), QSettings::IniFormat);
                settings.setValue("autoSaveInterval", interval->value());
                settings.setValue("keys/up", keys[0]);
                settings.setValue("keys/down", keys[1]);
                settings.setValue("keys/left", keys[2]);
                settings.setValue("keys/right", keys[3]);
                settings.setValue("keys/undo", keys[4]);
                settings.setValue("keys/save", keys[5]);
                settings.setValue("keys/restart", keys[6]);
                settings.setValue("keys/menu", keys[7]);
                settings.sync();
                loadKeyBindings();
                APP_LOG_INFO_MSG("settings", "auto save interval=%d", interval->value());
                message->setText("设置已保存。");
            });
    connect(dlcManagerButton, &QPushButton::clicked, this, &MainWindow::showDlcManager);
    connect(openModDirectoryButton, &QPushButton::clicked, this, [this]
            { QDesktopServices::openUrl(QUrl::fromLocalFile(QDir(modDirectoryPath()).absolutePath())); });
    if (adminSettingsButton != nullptr)
        connect(adminSettingsButton, &QPushButton::clicked, this, &MainWindow::showAdminDashboard);
    showDetailPage("设置", content);
}

void MainWindow::showDlcManager()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    const QString uid = currentUser.isEmpty() ? QString() : userUid(currentUser);
    auto *description = new QLabel(
        uid.isEmpty()
            ? "已激活 DLC 可以勾选加载；未激活 DLC 需要先输入 CD Key。"
            : QString("当前账号 UID：%1\n已激活 DLC 可以勾选加载；未激活 DLC 需要先输入 CD Key。")
                  .arg(uid));
    description->setProperty("role", "hint");
    description->setWordWrap(true);
    layout->addWidget(description);
    QList<QPair<QString, QCheckBox *>> choices;
    for (int index = 0; index < game_mode_count(); ++index)
    {
        const GameModeDefinition *mode = game_mode_at(index);
        if (mode == nullptr || QString::fromUtf8(mode->id) == "classic")
            continue;
        const QString id = QString::fromUtf8(mode->id);
        auto *check = new QCheckBox(QString("%1%2")
                                        .arg(QString::fromUtf8(mode->display_name))
                                        .arg(isDlcActivated(id) ? "" : "（未激活）"));
        check->setEnabled(isDlcActivated(id));
        check->setChecked(isDlcActivated(id));
        choices.append({id, check});
        layout->addWidget(check);
    }
    auto *message = new QLabel;
    message->setProperty("role", "hint");
    auto *addKeyButton = makeButton("添加 DLC CD Key");
    auto *saveButton = makeButton("保存 DLC 配置");
    layout->addWidget(message);
    layout->addWidget(addKeyButton);
    layout->addWidget(saveButton);
    layout->addStretch();
    connect(addKeyButton, &QPushButton::clicked, this, &MainWindow::showAddDlcKey);
    connect(saveButton, &QPushButton::clicked, content, [this, choices]
            {
        QStringList enabled;
        for (const auto &choice : choices)
            if (choice.second->isEnabled() && choice.second->isChecked())
                enabled.append(choice.first);
        QSettings settings(settingsFilePath(), QSettings::IniFormat);
        settings.setValue("dlc/enabled", enabled);
        if (!enabledModeIds().contains(settings.value("dlc/current", "classic").toString()))
            settings.setValue("dlc/current", "classic");
        settings.sync();
        QMessageBox::information(this, "DLC 管理", enabled.isEmpty() ? "没有可用的已激活 DLC。" : "DLC 配置已更新。");
        updateModeButton();
        showSettings(); });
    showDetailPage("DLC 管理", content, &MainWindow::showSettings);
}

void MainWindow::showAddDlcKey()
{
    if (guestMode || currentUser.isEmpty())
    {
        QMessageBox::warning(this, "无法激活", "请先使用普通账号登录后再激活 DLC。");
        showSettings();
        return;
    }
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *dlcName = new QLineEdit;
    auto *key = new QLineEdit;
    const QString uid = userUid(currentUser);
    auto *uidLabel = new QLabel(QString("当前账号 UID：%1").arg(uid.isEmpty() ? "未分配" : uid));
    uidLabel->setProperty("role", "hint");
    dlcName->setPlaceholderText("请输入 DLC 名称或模式 ID");
    key->setPlaceholderText("请输入管理员发放的 CD Key");
    auto *message = new QLabel;
    message->setProperty("role", "hint");
    auto *verify = makeButton("验证并添加");
    layout->addWidget(uidLabel);
    layout->addWidget(dlcName);
    layout->addWidget(key);
    layout->addWidget(message);
    layout->addWidget(verify);
    layout->addStretch();
    connect(verify, &QPushButton::clicked, content, [this, uid, dlcName, key, message]
            {
        const QString input = key->text().trimmed().toUpper();
        const QString dlcText = dlcName->text().trimmed();
        if (uid.isEmpty() || dlcText.isEmpty() || input.isEmpty())
        {
            message->setText("请填写 DLC 名称和 CD Key。");
            return;
        }
        QDir().mkpath(dlcDirectoryPath(uid));
        const QFileInfoList files = QDir(dlcDirectoryPath(uid))
                                        .entryInfoList(QStringList() << "*.DLC", QDir::Files, QDir::Time);
        QString storedName;
        QString dlcId;
        for (const QFileInfo &fileInfo : files)
        {
            QSettings registry(fileInfo.absoluteFilePath(), QSettings::IniFormat);
            const QString candidateName = registry.value("dlc_name").toString();
            const QString candidateId = registry.value("dlc_id").toString();
            if (candidateName == dlcText || candidateId == dlcText)
            {
                storedName = candidateName;
                dlcId = candidateId;
                break;
            }
        }
        if (storedName.isEmpty() || dlcId.isEmpty())
        {
            message->setText("未找到对应的 DLC 授权文件。");
            return;
        }
        if (!activateDlcKey(uid, dlcId, storedName, input))
        {
            message->setText("CD Key 无效，或与当前 UID / DLC 不匹配。");
            return;
        }
        const QString info = QString("%1 激活成功。状态已更新为 activated。")
                                 .arg(modeDisplayName(dlcId.toUtf8().constData()));
        message->setText(info);
        QMessageBox::information(this, "DLC 激活成功", info);
        showDlcManager(); });
    showDetailPage("添加 DLC CD Key", content, &MainWindow::showDlcManager);
}

void MainWindow::showHelp()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *help = new QLabel(
        "方向键或 W/A/S/D：移动方块\n\n"
        "撤销一步：恢复最近一次有效移动前的棋盘\n\n"
        "重新开始：放弃当前棋局并立即创建新棋局\n\n"
        "游客模式：可以完整游玩，但不保存存档、历史和排行榜成绩\n\n"
        "自动保存：默认每 50 次有效移动保存，可在设置中修改或关闭\n\n"
        "模式接口：核心层通过 start/process/judge 钩子支持计步和限时玩法；"
        "自定义棋盘尺寸需要动态棋盘结构");
    help->setWordWrap(true);
    help->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    help->setStyleSheet("font-size:16px;line-height:1.5;");
    layout->addWidget(help);
    layout->addStretch();
    showDetailPage("帮助", content);
}

bool MainWindow::isConfiguredAdmin(const QString &username) const
{
    QFile file(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/admins.dat");
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    while (!file.atEnd())
        if (QString::fromUtf8(file.readLine()).trimmed() == username)
            return true;
    return false;
}

void MainWindow::showAdminDashboard()
{
    if (!canOpenAdminSettings())
    {
        QMessageBox::warning(this, "操作失败", "当前账号不能进入管理员设置。");
        showMenu();
        return;
    }
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *welcome = new QLabel(QString("管理员：%1").arg(displayNameForUser(currentUser)));
    welcome->setAlignment(Qt::AlignCenter);
    welcome->setStyleSheet("font-size:20px;font-weight:700;");
    auto *admins = makeButton("添加与管理管理员");
    auto *system = makeButton("系统设置");
    auto *banUser = makeButton("封禁 / 解封用户");
    auto *issueDlc = makeButton("发放 DLC CD Key");
    auto *deleteUserButton = makeButton("删除用户");
    auto *logoutButton = makeButton("退出管理员账户");
    for (QPushButton *button : {admins, system, banUser, issueDlc, deleteUserButton, logoutButton})
        button->setStyleSheet("font-size:18px;font-weight:800;");
    layout->addWidget(welcome);
    if (isSuperAdminUser(currentUser))
    {
        layout->addWidget(admins);
        layout->addWidget(deleteUserButton);
    }
    layout->addWidget(system);
    layout->addWidget(banUser);
    layout->addWidget(issueDlc);
    layout->addWidget(logoutButton);
    layout->addStretch();
    connect(admins, &QPushButton::clicked, this, &MainWindow::showAdminManagement);
    connect(deleteUserButton, &QPushButton::clicked, this, &MainWindow::showAdminDeleteUser);
    connect(system, &QPushButton::clicked, this, &MainWindow::showSystemSettings);
    connect(banUser, &QPushButton::clicked, this, &MainWindow::showAdminBanUser);
    connect(issueDlc, &QPushButton::clicked, this, &MainWindow::showIssueDlcKey);
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
    showDetailPage("管理员控制台", content, &MainWindow::showAdminDashboard);
}

void MainWindow::showAdminManagement()
{
    if (!isSuperAdminUser(currentUser))
    {
        QMessageBox::warning(this, "操作失败", "只有超级管理员可以管理管理员权限。");
        showAdminDashboard();
        return;
    }
    const QString filePath = QString::fromUtf8(GAME_DATA_DIRECTORY) + "/admins.dat";
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *username = new QLineEdit;
    username->setPlaceholderText("输入已有普通账户用户名");
    auto *addButton = makeButton("添加管理员");
    auto *list = new QListWidget;
    QFile input(filePath);
    if (input.open(QIODevice::ReadOnly | QIODevice::Text))
        while (!input.atEnd())
        {
            const QString name = QString::fromUtf8(input.readLine()).trimmed();
            if (!name.isEmpty())
            {
                auto *item = new QListWidgetItem(displayNameForUser(name));
                item->setData(Qt::UserRole, name);
                list->addItem(item);
            }
        }
    auto *removeButton = makeButton("删除选中的管理员");
    auto *message = new QLabel;
    message->setProperty("role", "hint");
    layout->addWidget(username);
    layout->addWidget(addButton);
    layout->addWidget(new QLabel("管理员列表"));
    layout->addWidget(list);
    layout->addWidget(removeButton);
    layout->addWidget(message);
    connect(addButton, &QPushButton::clicked, content, [=]
            {
        const QString name = username->text().trimmed();
        if (!auth_user_exists(USER_DATA_FILE, name.toUtf8().constData()))
        {
            message->setText("无此账户，无法添加。");
            return;
        }
        if (isConfiguredAdmin(name))
        {
            message->setText("该账户已经是管理员。");
            return;
        }
        if (accountStatus(name) == "banned")
        {
            message->setText("已封禁用户不能设置为管理员。");
            return;
        }
        if (accountStatus(name) == "deleted")
        {
            message->setText("已注销用户不能设置为管理员。");
            return;
        }
        QFile output(filePath);
        if (output.open(QIODevice::Append | QIODevice::Text))
        {
            output.write(name.toUtf8() + "\n");
            auto *item = new QListWidgetItem(displayNameForUser(name));
            item->setData(Qt::UserRole, name);
            list->addItem(item);
            username->clear();
            message->setText("管理员已添加。");
        } });
    connect(removeButton, &QPushButton::clicked, content, [=]
            {
        if (list->currentItem() == nullptr)
            return;
        delete list->takeItem(list->currentRow());
        QFile output(filePath);
        if (output.open(QIODevice::WriteOnly | QIODevice::Text))
            for (int index = 0; index < list->count(); ++index)
                output.write(list->item(index)->data(Qt::UserRole).toString().toUtf8() + "\n");
        message->setText("管理员权限已删除。"); });
    showDetailPage("管理员管理", content, &MainWindow::showAdminDashboard);
}

void MainWindow::showAdminDeleteUser()
{
    if (!isSuperAdminUser(currentUser))
    {
        QMessageBox::warning(this, "操作失败", "只有超级管理员可以删除用户。");
        showAdminDashboard();
        return;
    }

    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *username = new QLineEdit;
    username->setPlaceholderText("输入要删除的普通用户用户名");
    auto *deleteButton = makeButton("删除用户");
    auto *message = new QLabel;
    message->setWordWrap(true);
    message->setProperty("role", "hint");
    layout->addWidget(username);
    layout->addWidget(deleteButton);
    layout->addWidget(message);
    layout->addStretch();
    connect(deleteButton, &QPushButton::clicked, content, [this, username, message]
            {
        const QString name = username->text().trimmed();
        if (name.isEmpty())
        {
            message->setText("请输入要删除的用户名。");
            return;
        }
        if (!auth_user_exists(USER_DATA_FILE, name.toUtf8().constData()))
        {
            message->setText("无此账户，无法删除。");
            return;
        }
        if (isSuperAdminUser(name) || name == currentUser)
        {
            message->setText("不能删除当前管理员或超级管理员账号。");
            return;
        }
        if (isConfiguredAdmin(name))
        {
            message->setText("管理员账号不能被删除，请先取消管理员权限后再操作。");
            return;
        }
        if (QMessageBox::warning(
                this, "删除用户",
                QString("确定要删除用户 %1 吗？该用户将无法登录，但历史记录和排行榜展示将按已注销用户处理。")
                    .arg(displayNameForUser(name)),
                QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
            return;
        deleteUser(name);
        if (accountStatus(name) == "deleted")
        {
            message->setText(QString("用户 %1 已删除。").arg(displayNameForUser(name)));
            username->clear();
        }
        else
        {
            message->setText("删除失败，请检查数据文件。");
        } });
    showDetailPage("删除用户", content, &MainWindow::showAdminDashboard);
}

void MainWindow::showAdminBanUser()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *username = new QLineEdit;
    username->setPlaceholderText("输入要封禁或解封的用户名");
    auto *status = new QLabel;
    status->setProperty("role", "hint");
    auto *banButton = makeButton("封禁用户");
    auto *unbanButton = makeButton("解除封禁");
    layout->addWidget(username);
    layout->addWidget(status);
    layout->addWidget(banButton);
    layout->addWidget(unbanButton);
    layout->addStretch();
    auto apply = [this, username, status](const QString &value)
    {
        const QString name = username->text().trimmed();
        if (!auth_user_exists(USER_DATA_FILE, name.toUtf8().constData()))
        {
            status->setText("无此账户。");
            return;
        }
        if (name == currentUser)
        {
            status->setText("不能封禁当前登录账号。");
            return;
        }
        if (isSuperAdminUser(name) || isConfiguredAdmin(name))
        {
            status->setText("管理员账号不能被封禁，请先取消管理员权限后再操作。");
            return;
        }
        if (accountStatus(name) == "deleted")
        {
            status->setText("已注销用户不能封禁或解封。");
            return;
        }
        if (value == "banned" && accountStatus(name) == "banned")
        {
            status->setText("该用户已被封禁。");
            return;
        }
        setAccountStatus(name, value);
        status->setText(value == "banned"
                            ? QString("用户 %1 已封禁。").arg(displayNameForUser(name))
                            : QString("用户 %1 已恢复正常。").arg(displayNameForUser(name)));
    };
    connect(banButton, &QPushButton::clicked, content, [apply]
            { apply("banned"); });
    connect(unbanButton, &QPushButton::clicked, content, [apply]
            { apply("normal"); });
    showDetailPage("封禁用户", content, &MainWindow::showAdminDashboard);
}

void MainWindow::showIssueDlcKey()
{
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout;
    auto *username = new QLineEdit;
    auto *mode = new QLineEdit;
    username->setPlaceholderText("输入目标用户用户名");
    mode->setPlaceholderText("输入 DLC 名称或模式 ID，例如 限时模式 / timed_300");
    form->addRow("目标用户名", username);
    form->addRow("DLC 名称 / ID", mode);
    auto *issue = makeButton("生成并发放 CD Key");
    auto *message = new QLabel;
    message->setWordWrap(true);
    message->setProperty("role", "hint");
    QStringList available;
    for (int index = 0; index < game_mode_count(); ++index)
    {
        const GameModeDefinition *definition = game_mode_at(index);
        if (definition != nullptr && QString::fromUtf8(definition->id) != "classic")
            available.append(QString("%1（%2）")
                                 .arg(QString::fromUtf8(definition->id),
                                      QString::fromUtf8(definition->display_name)));
    }
    auto *availableLabel = new QLabel("可用 DLC：" + (available.isEmpty() ? QString("暂无") : available.join("、")));
    availableLabel->setWordWrap(true);
    availableLabel->setProperty("role", "hint");
    auto *issuedTitle = new QLabel("已发放 CD Key 列表");
    issuedTitle->setStyleSheet("font-size:16px;font-weight:700;");
    auto *issuedTable = new QTableWidget;
    issuedTable->setColumnCount(5);
    issuedTable->setHorizontalHeaderLabels({"发放时间", "CD Key", "UID", "DLC 名称", "状态"});
    issuedTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    issuedTable->verticalHeader()->setVisible(false);
    issuedTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    issuedTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    issuedTable->setSelectionMode(QAbstractItemView::SingleSelection);
    issuedTable->setAlternatingRowColors(true);
    auto *copyKeyButton = makeButton("复制选中 CD Key");
    copyKeyButton->setEnabled(false);
    QVector<QJsonObject> issuedItems;
    QFile issuedFile(dlcIssuedListPath());
    if (issuedFile.open(QIODevice::ReadOnly))
    {
        const QJsonDocument doc = QJsonDocument::fromJson(issuedFile.readAll());
        issuedFile.close();
        for (const QJsonValue &value : doc.array())
            if (value.isObject())
                issuedItems.append(value.toObject());
    }
    std::sort(issuedItems.begin(), issuedItems.end(), [](const QJsonObject &left, const QJsonObject &right)
              { return left.value("issued_at").toVariant().toLongLong() >
                       right.value("issued_at").toVariant().toLongLong(); });
    issuedTable->setRowCount(issuedItems.size());
    for (int row = 0; row < issuedItems.size(); ++row)
    {
        const QJsonObject item = issuedItems[row];
        const qint64 issuedAt = item.value("issued_at").toVariant().toLongLong();
        const QString issuedText = issuedAt > 0
                                       ? QDateTime::fromSecsSinceEpoch(issuedAt).toString("yyyy-MM-dd HH:mm:ss")
                                       : "-";
        issuedTable->setItem(row, 0, new QTableWidgetItem(issuedText));
        issuedTable->setItem(row, 1, new QTableWidgetItem(item.value("plain_key").toString()));
        issuedTable->setItem(row, 2, new QTableWidgetItem(item.value("uid").toString()));
        issuedTable->setItem(row, 3, new QTableWidgetItem(item.value("dlc_name").toString()));
        issuedTable->setItem(row, 4, new QTableWidgetItem(item.value("status").toString()));
    }
    layout->addLayout(form);
    layout->addWidget(availableLabel);
    layout->addWidget(issue);
    layout->addWidget(message);
    layout->addWidget(issuedTitle);
    layout->addWidget(issuedTable);
    layout->addWidget(copyKeyButton);
    layout->addStretch();
    connect(issuedTable, &QTableWidget::itemSelectionChanged, content, [issuedTable, copyKeyButton]
            {
        copyKeyButton->setEnabled(issuedTable->currentRow() >= 0); });
    connect(copyKeyButton, &QPushButton::clicked, content, [issuedTable, message]
            {
        const int row = issuedTable->currentRow();
        if (row < 0 || issuedTable->item(row, 1) == nullptr)
            return;
        const QString key = issuedTable->item(row, 1)->text();
        QApplication::clipboard()->setText(key);
        message->setText(QString("已复制 CD Key：%1").arg(key)); });
    connect(issue, &QPushButton::clicked, content, [this, username, mode, message]
            {
        QString key;
        const QString usernameText = username->text().trimmed();
        QString modeText = mode->text().trimmed();
        QString modeId = modeText;
        const GameModeDefinition *definition = game_mode_find(modeId.toUtf8().constData());
        if (definition == nullptr)
        {
            for (int index = 0; index < game_mode_count(); ++index)
            {
                const GameModeDefinition *candidate = game_mode_at(index);
                if (candidate != nullptr && QString::fromUtf8(candidate->display_name) == modeText)
                {
                    modeId = QString::fromUtf8(candidate->id);
                    definition = candidate;
                    break;
                }
            }
        }
        if (definition == nullptr || !grantDlcKey(usernameText, modeId, &key))
        {
            message->setText("发放失败：请确认用户名存在、DLC 名称 / ID 正确且不是经典模式。");
            return;
        }
        const QString uidText = userUid(usernameText);
        const QString info = QString("已为 %1（UID：%2）发放 %3。\nCD Key：%4\n授权文件已写入该用户目录。")
                                 .arg(displayNameForUser(usernameText),
                                      uidText,
                                      modeDisplayName(modeId.toUtf8().constData()),
                                      key);
        QApplication::clipboard()->setText(key);
        message->setText(info);
        QMessageBox::information(this, "CD Key 已生成", info + "\n\nCD Key 已自动复制到剪贴板。");
        showIssueDlcKey(); });
    showDetailPage("发放 DLC CD Key", content, &MainWindow::showAdminDashboard);
}

void MainWindow::showSystemSettings()
{
    QSettings system(systemSettingsPath(), QSettings::IniFormat);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout;
    auto *renameHours = new QSpinBox;
    renameHours->setRange(1, 720);
    renameHours->setValue(system.value("usernameChangeCooldownHours",
                                       USERNAME_CHANGE_COOLDOWN_HOURS)
                              .toInt());
    auto *autoSave = new QSpinBox;
    autoSave->setRange(0, 1000);
    autoSave->setValue(system.value("defaultAutoSaveInterval", 50).toInt());
    form->addRow("用户名修改冷却（小时）", renameHours);
    form->addRow("新用户默认自动保存步数", autoSave);
    auto *save = makeButton("保存系统设置");
    layout->addLayout(form);
    layout->addWidget(save);
    layout->addStretch();
    connect(save, &QPushButton::clicked, content, [=]
            {
        QSettings settings(systemSettingsPath(), QSettings::IniFormat);
        settings.setValue("usernameChangeCooldownHours", renameHours->value());
        settings.setValue("defaultAutoSaveInterval", autoSave->value());
        settings.sync();
        QMessageBox::information(this, "系统设置", "系统设置已保存。");
        showAdminDashboard(); });
    showDetailPage("系统设置", content, &MainWindow::showAdminDashboard);
}

int MainWindow::usernameCooldownSeconds() const
{
    QSettings system(systemSettingsPath(), QSettings::IniFormat);
    return system.value("usernameChangeCooldownHours",
                        USERNAME_CHANGE_COOLDOWN_HOURS)
               .toInt() *
           3600;
}

int MainWindow::defaultAutoSaveInterval() const
{
    QSettings system(systemSettingsPath(), QSettings::IniFormat);
    return system.value("defaultAutoSaveInterval", 50).toInt();
}

void MainWindow::renderBoard()
{
    updateGameInfo();
    boardWidget->setBoard(board);
    undoButton->setEnabled(canUndo);
    saveButton->setEnabled(!guestMode);
    gameMessage->setText(guestMode
                             ? QString("游客模式 · %1 移动方块").arg(movementKeyText())
                             : QString("%1 移动方块").arg(movementKeyText()));
}

void MainWindow::updateGameInfo()
{
    scoreLabel->setText(QString("分数：%1").arg(board.score));
    stepLabel->setText(QString("步数：%1").arg(board.step));
    modeLabel->setText(QString("模式：%1").arg(modeDisplayName(board.mode)));
    updateTimeLabel();
}

void MainWindow::updateTimeLabel()
{
    if (timeLabel != nullptr)
        timeLabel->setText(QString("用时：%1").arg(formatTime(currentElapsedSeconds())));
}

void MainWindow::keyPressEvent(QKeyEvent *event)
{
    if (!gameActive || pages->currentWidget() != gamePage)
    {
        QMainWindow::keyPressEvent(event);
        return;
    }
    if (!handleConfiguredKey(event))
        QMainWindow::keyPressEvent(event);
}

bool MainWindow::handleConfiguredKey(QKeyEvent *event)
{
    const int pressed = event->keyCombination().toCombined();
    if (pressed == undoKey)
        undoMove();
    else if (pressed == saveKey)
        saveGame();
    else if (pressed == restartKey)
        restartGame();
    else if (pressed == menuKey)
        returnToMenu();
    else if (pressed == moveUpKey)
        handleMove(BOARD_CMD_UP);
    else if (pressed == moveDownKey)
        handleMove(BOARD_CMD_DOWN);
    else if (pressed == moveLeftKey)
        handleMove(BOARD_CMD_LEFT);
    else if (pressed == moveRightKey)
        handleMove(BOARD_CMD_RIGHT);
    else
        return false;
    event->accept();
    return true;
}

bool MainWindow::eventFilter(QObject *watched, QEvent *event)
{
    if (gameActive && pages->currentWidget() == gamePage &&
        event->type() == QEvent::KeyPress)
    {
        auto *keyEvent = static_cast<QKeyEvent *>(event);
        if (handleConfiguredKey(keyEvent))
            return true;
    }
    return QMainWindow::eventFilter(watched, event);
}

void MainWindow::handleMove(BoardCommand command)
{
    if (isCurrentUserBanned())
    {
        gameMessage->setText("当前账号已被封禁，不能继续游戏。");
        return;
    }
    updateTimeLabel();
    if (boardWidget->isAnimating())
    {
        queuedMove = command;
        hasQueuedMove = true;
        return;
    }

    Board previous = board;
    previous.elapsed_seconds = currentElapsedSeconds();
    if (!game_mode_process(board.mode, &board, command))
    {
        gameMessage->setText("这个方向无法移动。");
        updateTimeLabel();
        return;
    }
    undoBoard = previous;
    canUndo = true;
    hasUnsavedChanges = !guestMode;
    undoButton->setEnabled(false);
    updateGameInfo();
    boardWidget->animateMove(previous, board, command);
}

void MainWindow::finishAnimatedMove()
{
    if (!gameActive || pages->currentWidget() != gamePage)
        return;
    undoButton->setEnabled(canUndo);
    const int interval = autoSaveInterval();
    if (!guestMode && interval > 0 && board.step % interval == 0)
    {
        saveGame(false);
        gameMessage->setText(QString("已在第 %1 步自动保存。").arg(board.step));
    }
    Board judgedBoard = board;
    judgedBoard.elapsed_seconds = currentElapsedSeconds();
    const BoardStatus status = game_mode_judge(board.mode, &judgedBoard);
    if (status == BOARD_STATUS_WIN || status == BOARD_STATUS_LOSE)
    {
        hasQueuedMove = false;
        finishGame(status);
        return;
    }

    if (hasQueuedMove)
    {
        const BoardCommand command = queuedMove;
        hasQueuedMove = false;
        QTimer::singleShot(0, this, [this, command]
                           { handleMove(command); });
    }
}

void MainWindow::finishGame(BoardStatus status)
{
    board.elapsed_seconds = currentElapsedSeconds();
    gameTimer->stop();
    APP_LOG_INFO_MSG("game", "game finished user=%s status=%d score=%d step=%d time=%d mode=%s guest=%d",
                     currentUser.toUtf8().constData(), (int)status, board.score,
                     board.step, board.elapsed_seconds, board.mode, guestMode ? 1 : 0);
    bool rankSaved = false;
    bool historySaved = false;
    if (!guestMode && accountStatus(currentUser) != "banned")
    {
        const QByteArray username = currentUser.toUtf8();
        const GameModeDefinition *mode = game_mode_find(board.mode);
        if (mode == nullptr || mode->ranking_enabled)
        {
            rankSaved = rank_save_score(SCORES_DATA_FILE, username.constData(),
                                        board.score, board_get_max_tile(&board),
                                        board.step, board.elapsed_seconds, board.mode);
        }
        HistoryEntry entry = {};
        utils_copy_string(entry.username, username.constData(), sizeof(entry.username));
        entry.score = board.score;
        entry.max_tile = board_get_max_tile(&board);
        entry.steps = board.step;
        entry.elapsed_seconds = board.elapsed_seconds;
        utils_copy_string(entry.mode, board.mode, sizeof(entry.mode));
        entry.finished_at = QDateTime::currentSecsSinceEpoch();
        historySaved = history_save(historyFilePath().toUtf8().constData(), &entry);
        (void)storage_delete_save(saveFilePath().toUtf8().constData(), username.constData());
    }
    const QString recordText = guestMode
                                   ? "游客成绩不计入排行榜和历史记录。"
                                   : QString("排行榜：%1，历史记录：%2")
                                         .arg(rankSaved ? "已更新" : "未更新")
                                         .arg(historySaved ? "已保存" : "保存失败");
    QMessageBox::information(
        this,
        status == BOARD_STATUS_WIN ? "游戏胜利" : "游戏结束",
        QString("%1\n最终分数：%2\n最大方块：%3\n步数：%4\n用时：%5\n%6")
            .arg(status == BOARD_STATUS_WIN ? "恭喜你合成了 2048！"
                                            : "棋盘已经没有可移动位置。")
            .arg(board.score)
            .arg(board_get_max_tile(&board))
            .arg(board.step)
            .arg(formatTime(board.elapsed_seconds))
            .arg(recordText));
    hasUnsavedChanges = false;
    showMenu();
}

int MainWindow::currentElapsedSeconds() const
{
    if (!gameActive || !sessionTimer.isValid())
        return board.elapsed_seconds;
    return board.elapsed_seconds + static_cast<int>(sessionTimer.elapsed() / 1000);
}

int MainWindow::autoSaveInterval() const
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    return settings.value("autoSaveInterval", defaultAutoSaveInterval()).toInt();
}

void MainWindow::loadKeyBindings()
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    moveUpKey = settings.value("keys/up", Qt::Key_W).toInt();
    moveDownKey = settings.value("keys/down", Qt::Key_S).toInt();
    moveLeftKey = settings.value("keys/left", Qt::Key_A).toInt();
    moveRightKey = settings.value("keys/right", Qt::Key_D).toInt();
    undoKey = settings.value("keys/undo", Qt::Key_Z).toInt();
    saveKey = settings.value("keys/save", Qt::Key_C).toInt();
    restartKey = settings.value("keys/restart", Qt::Key_R).toInt();
    menuKey = settings.value("keys/menu", Qt::Key_B).toInt();
}

QString MainWindow::settingsFilePath() const
{
    return userDirectoryPath() + "/settings.ini";
}

QString MainWindow::userDirectoryPath() const
{
    const QString root = QString::fromUtf8(GAME_DATA_DIRECTORY) + "/Users";
    QDir().mkpath(root);
    if (guestMode || currentUser.isEmpty())
    {
        QDir().mkpath(root + "/guest");
        return root + "/guest";
    }
    const QString path = userDirectoryForUsername(currentUser);
    QDir().mkpath(path);
    return path;
}

QString MainWindow::userDirectoryForUsername(const QString &usernameText) const
{
    const QString root = QString::fromUtf8(GAME_DATA_DIRECTORY) + "/Users";
    const QString uid = userUid(usernameText);
    if (!uid.isEmpty())
        return root + "/" + uid;
    char encrypted[USER_NAME_LENGTH_MAX * 2 + 16] = {0};
    const QByteArray username = usernameText.toUtf8();
    if (!utils_xor_encrypt_to_hex(username.constData(), encrypted, sizeof(encrypted)))
        return root + "/invalid";
    return root + "/" + QString::fromLatin1(encrypted);
}

QString MainWindow::userUid(const QString &username) const
{
    QSettings accounts(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/accounts.ini",
                       QSettings::IniFormat);
    return accounts.value("users/" + username).toString();
}

QString MainWindow::usernameForUid(const QString &uid) const
{
    if (uid.isEmpty())
        return QString();
    QSettings accounts(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/accounts.ini",
                       QSettings::IniFormat);
    accounts.beginGroup("users");
    const QStringList usernames = accounts.childKeys();
    for (const QString &username : usernames)
        if (accounts.value(username).toString() == uid)
        {
            accounts.endGroup();
            return username;
        }
    accounts.endGroup();
    return QString();
}

QString MainWindow::ensureUserUid(const QString &username) const
{
    if (username.isEmpty() || username == "游客" || username == "Administrator")
        return QString();
    QSettings accounts(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/accounts.ini",
                       QSettings::IniFormat);
    QString uid = accounts.value("users/" + username).toString();
    if (!uid.isEmpty())
        return uid;

    int next = accounts.value("meta/nextUid", 1).toInt();
    QString candidate;
    do
    {
        candidate = QString("%1").arg(next, 6, 10, QChar('0'));
        ++next;
    } while (QDir(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/Users/" + candidate).exists());
    accounts.setValue("users/" + username, candidate);
    accounts.setValue("status/" + username, "normal");
    accounts.setValue("meta/nextUid", next);
    accounts.sync();
    return candidate;
}

QString MainWindow::accountStatus(const QString &username) const
{
    if (username == "Administrator")
        return "admin";
    QSettings accounts(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/accounts.ini",
                       QSettings::IniFormat);
    const QString stored = accounts.value("status/" + username, "normal").toString();
    if (stored == "deleted" || stored == "banned")
        return stored;
    if (isConfiguredAdmin(username))
        return "admin";
    return stored;
}

QString MainWindow::accountStatusText(const QString &username, bool deleted) const
{
    if (deleted || accountStatus(username) == "deleted")
        return "账户异常";
    if (accountStatus(username) == "banned")
        return "账户异常";
    if (accountStatus(username) == "admin")
        return "管理员";
    return "正常";
}

void MainWindow::setAccountStatus(const QString &username, const QString &status) const
{
    if (username.isEmpty() || username == "Administrator")
        return;
    QSettings accounts(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/accounts.ini",
                       QSettings::IniFormat);
    accounts.setValue("status/" + username, status);
    accounts.sync();
}

QString MainWindow::saveFilePath() const
{
    return userDirectoryPath() + "/save.dat";
}

QString MainWindow::historyFilePath() const
{
    return userDirectoryPath() + "/history.dat";
}

QString MainWindow::profileFilePath() const
{
    return userDirectoryPath() + "/profile.ini";
}

QString MainWindow::modDirectoryPath() const
{
    return QString::fromUtf8(GAME_DATA_DIRECTORY) + "/DLC";
}

QString MainWindow::systemSettingsPath() const
{
    const QString path = QString::fromUtf8(GAME_DATA_DIRECTORY) + "/System";
    QDir().mkpath(path);
    return path + "/system.ini";
}

void MainWindow::migrateLegacyUserData()
{
    if (guestMode || currentUser.isEmpty())
        return;
    const QString uid = ensureUserUid(currentUser);
    const QString root = QString::fromUtf8(GAME_DATA_DIRECTORY) + "/Users";
    char encryptedName[USER_NAME_LENGTH_MAX * 2 + 16] = {0};
    const QByteArray currentName = currentUser.toUtf8();
    if (!uid.isEmpty() &&
        utils_xor_encrypt_to_hex(currentName.constData(), encryptedName, sizeof(encryptedName)))
    {
        const QString legacyDirectory = root + "/" + QString::fromLatin1(encryptedName);
        const QString uidDirectory = root + "/" + uid;
        QDir().mkpath(uidDirectory);
        if (legacyDirectory != uidDirectory && QDir(legacyDirectory).exists())
        {
            for (const QString &file : QDir(legacyDirectory).entryList(QDir::Files))
                if (!QFile::exists(uidDirectory + "/" + file))
                    (void)QFile::rename(legacyDirectory + "/" + file, uidDirectory + "/" + file);
            (void)QDir(legacyDirectory).removeRecursively();
        }
    }
    const QByteArray username = currentUser.toUtf8();
    const QByteArray userSave = saveFilePath().toUtf8();
    Board legacyBoard = {};
    if (!storage_save_exists(userSave.constData(), username.constData()) &&
        storage_load_save(SAVES_DATA_FILE, username.constData(), &legacyBoard))
    {
        (void)storage_save_game(userSave.constData(), username.constData(), &legacyBoard);
        (void)storage_delete_save(SAVES_DATA_FILE, username.constData());
    }
    HistoryEntry entries[HISTORY_ENTRIES_MAX];
    const int count = history_load_user(HISTORY_DATA_FILE, username.constData(),
                                        entries, HISTORY_ENTRIES_MAX);
    if (count > 0)
    {
        const QByteArray userHistory = historyFilePath().toUtf8();
        for (int index = 0; index < count; ++index)
            (void)history_save(userHistory.constData(), &entries[index]);
        (void)history_delete_user(HISTORY_DATA_FILE, username.constData());
    }
    const QString legacySettings = QString::fromUtf8(GAME_DATA_DIRECTORY) +
                                   "/Settings/" +
                                   QString::fromLatin1(username.toHex()) + ".ini";
    if (!QFile::exists(settingsFilePath()) && QFile::exists(legacySettings))
        (void)QFile::rename(legacySettings, settingsFilePath());
}

QStringList MainWindow::enabledModeIds() const
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    QStringList result = {"classic"};
    const QStringList authorized = authorizedDlcIds();
    const QStringList enabled = settings.value("dlc/enabled").toStringList();
    for (const QString &id : enabled)
        if (authorized.contains(id) && game_mode_find(id.toUtf8().constData()) != nullptr &&
            !result.contains(id))
            result.append(id);
    return result;
}

QStringList MainWindow::authorizedDlcIds() const
{
    if (guestMode || currentUser.isEmpty())
        return {};
    QStringList result;
    const QDir dir(dlcDirectoryPath(userUid(currentUser)));
    const QFileInfoList files = dir.entryInfoList(QStringList() << "*.DLC", QDir::Files, QDir::Time);
    for (const QFileInfo &fileInfo : files)
    {
        QSettings auth(fileInfo.absoluteFilePath(), QSettings::IniFormat);
        if (auth.value("status").toString() == "activated")
            result.append(auth.value("dlc_id").toString());
    }
    result.removeDuplicates();
    return result;
}

QStringList MainWindow::activatedDlcIds() const
{
    return authorizedDlcIds();
}

bool MainWindow::isDlcAuthorized(const QString &modeId) const
{
    return modeId == "classic" || activatedDlcIds().contains(modeId);
}

bool MainWindow::isDlcActivated(const QString &modeId) const
{
    return modeId == "classic" || activatedDlcIds().contains(modeId);
}

QString MainWindow::dlcDirectoryPath(const QString &uid) const
{
    return QString::fromUtf8(GAME_DATA_DIRECTORY) + "/Users/" + uid + "/DLC";
}

QString MainWindow::dlcFilePath(const QString &uid, const QString &modeId) const
{
    return dlcDirectoryPath(uid) + "/" + modeId + ".DLC";
}

QString MainWindow::dlcIssuedListPath() const
{
    return QString::fromUtf8(GAME_DATA_DIRECTORY) + "/DLC/issued_keys.json";
}

QString MainWindow::dlcKeyHash(const QString &dlcName, const QString &uid, const QString &key) const
{
    const QString salt = dlcName + uid;
    const QByteArray text = QString("%1:%2").arg(salt, key).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(text, QCryptographicHash::Sha256).toHex());
}

QString MainWindow::generateDlcKey(const QString &uid, const QString &modeId) const
{
    const QByteArray seed = QString("%1:%2:%3").arg(uid, modeId, QString::number(QDateTime::currentMSecsSinceEpoch())).toUtf8();
    return QString::fromLatin1(QCryptographicHash::hash(seed, QCryptographicHash::Sha256)
                                   .toHex()
                                   .left(16))
        .toUpper();
}

bool MainWindow::grantDlcKey(const QString &username, const QString &modeId, QString *plainKey) const
{
    const QString resolvedUsername = username.trimmed();
    if (resolvedUsername.isEmpty() ||
        !auth_user_exists(USER_DATA_FILE, resolvedUsername.toUtf8().constData()) ||
        game_mode_find(modeId.toUtf8().constData()) == nullptr ||
        modeId == "classic")
        return false;
    const QString uid = userUid(resolvedUsername);
    if (uid.isEmpty())
        return false;
    if (accountStatus(resolvedUsername) == "deleted")
        return false;
    const QString key = generateDlcKey(uid, modeId);
    const QString filePath = dlcFilePath(uid, modeId);
    QDir().mkpath(dlcDirectoryPath(uid));
    QSettings registry(filePath, QSettings::IniFormat);
    registry.setValue("dlc_id", modeId);
    registry.setValue("dlc_name", modeDisplayName(modeId.toUtf8().constData()));
    registry.setValue("uid", uid);
    registry.setValue("key_hash", dlcKeyHash(modeDisplayName(modeId.toUtf8().constData()), uid, key));
    registry.setValue("status", "issued");
    registry.setValue("issued_at", QDateTime::currentSecsSinceEpoch());
    registry.sync();
    QJsonArray list;
    QFile listFile(dlcIssuedListPath());
    if (listFile.open(QIODevice::ReadOnly))
    {
        const QJsonDocument doc = QJsonDocument::fromJson(listFile.readAll());
        list = doc.array();
        listFile.close();
    }
    QJsonObject item;
    item["dlc_id"] = modeId;
    item["dlc_name"] = modeDisplayName(modeId.toUtf8().constData());
    item["uid"] = uid;
    item["plain_key"] = key;
    item["status"] = "issued";
    item["issued_at"] = QDateTime::currentSecsSinceEpoch();
    list.append(item);
    QDir().mkpath(QFileInfo(dlcIssuedListPath()).absolutePath());
    QFile out(dlcIssuedListPath());
    if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
        out.write(QJsonDocument(list).toJson(QJsonDocument::Compact));
    if (plainKey != nullptr)
        *plainKey = key;
    return true;
}

bool MainWindow::activateDlcKey(const QString &uid, const QString &modeId, const QString &dlcName, const QString &plainKey) const
{
    if (uid.isEmpty() || modeId.isEmpty() || plainKey.isEmpty())
        return false;
    QSettings registry(dlcFilePath(uid, modeId), QSettings::IniFormat);
    if (registry.value("uid").toString() != uid ||
        registry.value("dlc_id").toString() != modeId ||
        registry.value("dlc_name").toString() != dlcName)
        return false;
    if (registry.value("key_hash").toString() != dlcKeyHash(dlcName, uid, plainKey))
        return false;
    registry.setValue("status", "activated");
    registry.sync();
    QFile listFile(dlcIssuedListPath());
    if (listFile.open(QIODevice::ReadOnly))
    {
        QJsonArray list = QJsonDocument::fromJson(listFile.readAll()).array();
        listFile.close();
        bool changed = false;
        for (int index = 0; index < list.size(); ++index)
        {
            if (!list[index].isObject())
                continue;
            QJsonObject item = list[index].toObject();
            if (item.value("uid").toString() == uid &&
                item.value("dlc_id").toString() == modeId &&
                item.value("plain_key").toString() == plainKey)
            {
                item["status"] = "activated";
                item["activated_at"] = QDateTime::currentSecsSinceEpoch();
                list[index] = item;
                changed = true;
            }
        }
        if (changed)
        {
            QDir().mkpath(QFileInfo(dlcIssuedListPath()).absolutePath());
            QFile out(dlcIssuedListPath());
            if (out.open(QIODevice::WriteOnly | QIODevice::Truncate))
                out.write(QJsonDocument(list).toJson(QJsonDocument::Compact));
        }
    }
    return true;
}

QString MainWindow::selectedModeId() const
{
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    const QString selected = settings.value("dlc/current", "classic").toString();
    return enabledModeIds().contains(selected) ? selected : QStringLiteral("classic");
}

void MainWindow::cycleGameMode()
{
    const QStringList modes = enabledModeIds();
    const int current = modes.indexOf(selectedModeId());
    const QString next = modes[(current + 1) % modes.size()];
    QSettings settings(settingsFilePath(), QSettings::IniFormat);
    settings.setValue("dlc/current", next);
    settings.sync();
    updateModeButton();
}

void MainWindow::updateModeButton()
{
    if (modeButton == nullptr)
        return;
    const QByteArray id = selectedModeId().toUtf8();
    modeButton->setText(modeDisplayName(id.constData()));
    modeButton->setVisible(enabledModeIds().size() > 1);
}

QString MainWindow::movementKeyText() const
{
    const auto name = [](int key)
    {
        return QKeySequence(key).toString(QKeySequence::NativeText);
    };
    return QString("%1/%2/%3/%4")
        .arg(name(moveUpKey), name(moveDownKey), name(moveLeftKey), name(moveRightKey));
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (gameActive && !guestMode && !currentUser.isEmpty() && hasUnsavedChanges)
        saveGame(false);
    clearSessionPassword();
    QMainWindow::closeEvent(event);
}

QString MainWindow::authMessage(int result) const
{
    switch (static_cast<AuthResult>(result))
    {
    case AUTH_INVALID_USERNAME:
        return "用户名需为 2-31 位字母、数字或下划线。";
    case AUTH_INVALID_PASSWORD:
        return "密码需为 6-20 位且不能包含空白字符。";
    case AUTH_USERNAME_EXISTS:
        return "该用户名已经注册。";
    case AUTH_USER_NOT_FOUND:
        return "用户不存在。";
    case AUTH_WRONG_PASSWORD:
        return "密码错误。";
    case AUTH_FILE_ERROR:
        return "数据文件读写失败。";
    default:
        return {};
    }
}

bool MainWindow::isSuperAdminUser(const QString &username) const
{
    return username == "Administrator";
}

bool MainWindow::isAdminUser(const QString &username) const
{
    return isSuperAdminUser(username) || isConfiguredAdmin(username);
}

bool MainWindow::isCurrentUserBanned() const
{
    return !guestMode && accountStatus(currentUser) == "banned";
}

bool MainWindow::canOpenAdminSettings() const
{
    if (guestMode)
        return false;
    const QString status = accountStatus(currentUser);
    if (status == "banned" || status == "deleted")
        return false;
    return isAdminUser(currentUser);
}

QString MainWindow::displayNameForUser(const QString &username) const
{
    const QString status = accountStatus(username);
    if (status == "deleted")
        return QStringLiteral("（已注销）") + username;
    if (status == "banned")
        return QStringLiteral("（已封禁）") + username;
    return username;
}

void MainWindow::deleteUser(const QString &username)
{
    if (username.isEmpty() || username == "Administrator")
        return;
    if (!auth_user_exists(USER_DATA_FILE, username.toUtf8().constData()))
        return;
    if (isConfiguredAdmin(username))
        return;

    const QByteArray userBytes = username.toUtf8();
    const AuthResult result = auth_admin_delete_user(USER_DATA_FILE, userBytes.constData());
    if (result != AUTH_OK)
    {
        QMessageBox::warning(this, "删除失败", authMessage(result));
        return;
    }
    setAccountStatus(username, "deleted");
    (void)rank_mark_deleted(SCORES_DATA_FILE, userBytes.constData());
    APP_LOG_INFO_MSG("admin", "user deleted by admin: %s", userBytes.constData());
}

QString MainWindow::modeDisplayName(const char *mode) const
{
    const GameModeDefinition *definition = game_mode_find(
        mode == nullptr || mode[0] == '\0' ? "classic" : mode);
    return definition == nullptr ? QString::fromUtf8(mode)
                                 : QString::fromUtf8(definition->display_name);
}
