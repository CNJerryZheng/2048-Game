#include "mainwindow.h"
#include "boardwidget.h"

extern "C"
{
#include "auth.h"
#include "config.h"
#include "game_mode.h"
#include "history.h"
#include "rank.h"
#include "storage.h"
#include "utils.h"
}

#include <QByteArray>
#include <QCloseEvent>
#include <QDateTime>
#include <QColor>
#include <QDir>
#include <QFormLayout>
#include <QFont>
#include <QGridLayout>
#include <QHeaderView>
#include <QKeyEvent>
#include <QLabel>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QSpinBox>
#include <QStackedWidget>
#include <QTableWidget>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

namespace
{
    QPushButton *makeButton(const QString &text)
    {
        auto *button = new QPushButton(text);
        button->setMinimumHeight(42);
        button->setCursor(Qt::PointingHandCursor);
        return button;
    }

    QString formatTime(int totalSeconds)
    {
        const int hours = totalSeconds / 3600;
        const int minutes = totalSeconds % 3600 / 60;
        const int seconds = totalSeconds % 60;
        return hours > 0
                   ? QString("%1:%2:%3").arg(hours).arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0'))
                   : QString("%1:%2").arg(minutes, 2, 10, QLatin1Char('0')).arg(seconds, 2, 10, QLatin1Char('0'));
    }
}

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    QDir().mkpath(QString::fromUtf8(GAME_DATA_DIRECTORY));
    QDir().mkpath(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/Mods");
    setWindowTitle("2048 Game");
    setMinimumSize(620, 720);

    pages = new QStackedWidget(this);
    setCentralWidget(pages);
    buildAuthPage();
    buildMenuPage();
    buildGamePage();
    buildDetailPage();
    pages->setCurrentWidget(authPage);

    gameTimer = new QTimer(this);
    gameTimer->setInterval(1000);
    connect(gameTimer, &QTimer::timeout, this, &MainWindow::updateTimeLabel);

    setStyleSheet(
        "QMainWindow{background:#faf8ef;}"
        "QStackedWidget{background:#faf8ef;}"
        "QLabel{color:#776e65;}"
        "QLineEdit,QSpinBox{padding:9px;border:1px solid #bbada0;border-radius:6px;"
        "background:#ffffff;color:#3c3a32;selection-background-color:#8f7a66;"
        "selection-color:#ffffff;}"
        "QLineEdit:focus,QSpinBox:focus{border:2px solid #8f7a66;}"
        "QPushButton{background:#edc22e;color:#3c3a32;border:1px solid #c9a51f;"
        "border-radius:6px;padding:8px 16px;font-weight:700;}"
        "QPushButton:hover{background:#f2cf48;}"
        "QPushButton:pressed{background:#d8ad1f;}"
        "QPushButton:disabled{background:#ddd5cc;color:#776e65;border-color:#c8beb4;}"
        "QTabWidget::pane{border:0;}"
        "QTabBar::tab{color:#3c3a32;background:#fffdf8;border:1px solid #ded6cc;"
        "padding:8px 16px;margin-right:2px;}"
        "QTabBar::tab:selected{color:#3c3a32;background:#f2cf48;border-color:#d8ad1f;}"
        "QTabBar::tab:hover:!selected{background:#f5efe6;}"
        "QTableWidget{background:#fffdf8;color:#3c3a32;border:1px solid #ded6cc;"
        "border-radius:8px;gridline-color:#e8e1d8;alternate-background-color:#f7f3ed;"
        "selection-background-color:#f8e59a;selection-color:#3c3a32;}"
        "QTableWidget::item{color:#3c3a32;padding:5px;}"
        "QHeaderView::section{background:#eee8df;color:#3c3a32;border:0;"
        "border-right:1px solid #ded6cc;border-bottom:1px solid #ded6cc;"
        "padding:7px;font-weight:700;}"
        "QTableCornerButton::section{background:#eee8df;border:0;}");
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
    loginUsername->setPlaceholderText("请输入2-31位字母、数字或下划线");
    loginPassword->setPlaceholderText("请输入6-20位密码");
    loginPassword->setEchoMode(QLineEdit::Password);
    loginMessage = new QLabel;
    loginMessage->setWordWrap(true);
    auto *loginButton = makeButton("登录");
    auto *guestButton = makeButton("游客进入（成绩不保存）");
    loginLayout->addRow("用户名", loginUsername);
    loginLayout->addRow("密码", loginPassword);
    loginLayout->addRow(loginMessage);
    loginLayout->addRow(loginButton);
    loginLayout->addRow(guestButton);
    authTabs->addTab(loginTab, "登录");

    auto *registerTab = new QWidget;
    auto *registerLayout = new QFormLayout(registerTab);
    registerUsername = new QLineEdit;
    registerPassword = new QLineEdit;
    registerConfirmation = new QLineEdit;
    registerUsername->setPlaceholderText("请输入2-31位字母、数字或下划线");
    registerPassword->setPlaceholderText("请输入6-20位密码");
    registerConfirmation->setPlaceholderText("请再次输入密码");
    registerPassword->setEchoMode(QLineEdit::Password);
    registerConfirmation->setEchoMode(QLineEdit::Password);
    registerMessage = new QLabel;
    registerMessage->setWordWrap(true);
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
    pages->addWidget(menuPage);

    connect(newGameButton, &QPushButton::clicked, this, &MainWindow::beginNewGame);
    connect(continueButton, &QPushButton::clicked, this, &MainWindow::startSavedGame);
    connect(rankingButton, &QPushButton::clicked, this, &MainWindow::showRanking);
    connect(historyButton, &QPushButton::clicked, this, &MainWindow::showHistory);
    connect(userCenterButton, &QPushButton::clicked, this, &MainWindow::showUserCenter);
    connect(settingsButton, &QPushButton::clicked, this, &MainWindow::showSettings);
    connect(helpButton, &QPushButton::clicked, this, &MainWindow::showHelp);
    connect(logoutButton, &QPushButton::clicked, this, &MainWindow::logout);
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

void MainWindow::showDetailPage(const QString &titleText, QWidget *content)
{
    while (QLayoutItem *item = detailLayout->takeAt(0))
    {
        delete item->widget();
        delete item;
    }

    auto *backButton = makeButton("返回主菜单");
    backButton->setMaximumWidth(150);
    auto *title = new QLabel(titleText);
    title->setAlignment(Qt::AlignCenter);
    title->setStyleSheet("font-size:30px;font-weight:800;color:#776e65;");
    detailLayout->addWidget(backButton, 0, Qt::AlignLeft);
    detailLayout->addWidget(title);
    detailLayout->addWidget(content, 1);
    connect(backButton, &QPushButton::clicked, this, &MainWindow::showMenu);
    pages->setCurrentWidget(detailPage);
}

void MainWindow::login()
{
    const QByteArray username = loginUsername->text().trimmed().toUtf8();
    const QByteArray password = loginPassword->text().toUtf8();
    const AuthResult result = auth_login_user(USER_DATA_FILE,
                                              username.constData(),
                                              password.constData());
    if (result != AUTH_OK)
    {
        loginMessage->setText(authMessage(result));
        return;
    }
    currentUser = QString::fromUtf8(username);
    guestMode = false;
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
        loginUsername->setText(registerUsername->text().trimmed());
        registerPassword->clear();
        registerConfirmation->clear();
        authTabs->setCurrentIndex(0);
    }
}

void MainWindow::enterAsGuest()
{
    currentUser = "游客";
    guestMode = true;
    showMenu();
}

void MainWindow::showMenu()
{
    gameActive = false;
    hasQueuedMove = false;
    if (boardWidget != nullptr)
        boardWidget->cancelAnimation();
    gameTimer->stop();
    sessionTimer.invalidate();
    welcomeLabel->setText(guestMode ? "游客模式 · 成绩与存档不会保存"
                                    : QString("欢迎，%1").arg(currentUser));
    const QByteArray username = currentUser.toUtf8();
    continueButton->setEnabled(!guestMode && storage_save_exists(
                                                 SAVES_DATA_FILE, username.constData()));
    userCenterButton->setEnabled(!guestMode);
    historyButton->setEnabled(!guestMode);
    pages->setCurrentWidget(menuPage);
}

void MainWindow::beginNewGame()
{
    const QByteArray username = currentUser.toUtf8();
    if (!guestMode && storage_save_exists(SAVES_DATA_FILE, username.constData()))
    {
        const auto answer = QMessageBox::question(
            this, "开始新游戏", "开始新游戏会覆盖当前存档，是否继续？");
        if (answer != QMessageBox::Yes)
            return;
        (void)storage_delete_save(SAVES_DATA_FILE, username.constData());
    }
    board_start(&board);
    canUndo = false;
    hasQueuedMove = false;
    gameActive = true;
    sessionTimer.start();
    gameTimer->start();
    pages->setCurrentWidget(gamePage);
    renderBoard();
}

void MainWindow::startSavedGame()
{
    if (guestMode)
        return;
    const QByteArray username = currentUser.toUtf8();
    if (!storage_load_save(SAVES_DATA_FILE, username.constData(), &board))
    {
        QMessageBox::warning(this, "读取失败", "没有找到有效存档。");
        showMenu();
        return;
    }
    board.game_over = false;
    canUndo = false;
    hasQueuedMove = false;
    gameActive = true;
    sessionTimer.start();
    gameTimer->start();
    pages->setCurrentWidget(gamePage);
    renderBoard();
}

void MainWindow::saveGame(bool showMessage)
{
    if (guestMode)
    {
        if (showMessage)
            gameMessage->setText("游客模式不保存游戏。");
        return;
    }
    board.elapsed_seconds = currentElapsedSeconds();
    sessionTimer.restart();
    const QByteArray username = currentUser.toUtf8();
    const bool success = storage_save_game(SAVES_DATA_FILE,
                                           username.constData(), &board);
    if (showMessage)
        gameMessage->setText(success ? "游戏已加密保存。"
                                     : "保存失败，请检查数据目录。");
}

void MainWindow::returnToMenu()
{
    hasQueuedMove = false;
    boardWidget->cancelAnimation();
    if (!guestMode)
        saveGame(false);
    showMenu();
}

void MainWindow::logout()
{
    if (gameActive && !guestMode)
        saveGame(false);
    currentUser.clear();
    guestMode = false;
    gameActive = false;
    gameTimer->stop();
    pages->setCurrentWidget(authPage);
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
    sessionTimer.restart();
    renderBoard();
    gameMessage->setText("已撤销上一步。");
}

void MainWindow::restartGame()
{
    if (QMessageBox::question(this, "重新开始",
                              "确定放弃当前进度并重新开始吗？") != QMessageBox::Yes)
        return;
    hasQueuedMove = false;
    boardWidget->cancelAnimation();
    if (!guestMode)
    {
        const QByteArray username = currentUser.toUtf8();
        (void)storage_delete_save(SAVES_DATA_FILE, username.constData());
    }
    board_start(&board);
    canUndo = false;
    sessionTimer.restart();
    renderBoard();
}

void MainWindow::showRanking()
{
    RankEntry entries[RANK_ENTRIES_MAX];
    const int count = rank_load_scores(SCORES_DATA_FILE,
                                       entries, RANK_ENTRIES_MAX);
    const int visibleCount = qMin(count, RANK_TOP_COUNT);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *table = new QTableWidget(visibleCount, 7);
    table->setAlternatingRowColors(true);
    table->setHorizontalHeaderLabels(
        {"排名", "用户名", "分数", "最大方块", "步数", "用时", "模式"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionMode(QAbstractItemView::NoSelection);
    int currentRank = -1;
    int currentScore = 0;
    for (int index = 0; index < count; ++index)
    {
        if (!guestMode && currentUser == QString::fromUtf8(entries[index].username))
        {
            currentRank = index + 1;
            currentScore = entries[index].score;
        }
        if (index >= visibleCount)
            continue;
        table->setItem(index, 0, new QTableWidgetItem(QString::number(index + 1)));
        table->setItem(index, 1, new QTableWidgetItem(QString::fromUtf8(entries[index].username)));
        table->setItem(index, 2, new QTableWidgetItem(QString::number(entries[index].score)));
        table->setItem(index, 3, new QTableWidgetItem(QString::number(entries[index].max_tile)));
        table->setItem(index, 4, new QTableWidgetItem(QString::number(entries[index].steps)));
        table->setItem(index, 5, new QTableWidgetItem(formatTime(entries[index].elapsed_seconds)));
        table->setItem(index, 6, new QTableWidgetItem(modeDisplayName(entries[index].mode)));
        if (!guestMode && currentUser == QString::fromUtf8(entries[index].username))
        {
            for (int column = 0; column < table->columnCount(); ++column)
            {
                table->item(index, column)->setBackground(QColor("#fff0a6"));
                QFont font = table->item(index, column)->font();
                font.setBold(true);
                table->item(index, column)->setFont(font);
            }
        }
    }
    layout->addWidget(table);
    if (!guestMode && currentRank > RANK_TOP_COUNT)
    {
        const int tenthScore = entries[RANK_TOP_COUNT - 1].score;
        const int difference = qMax(0, tenthScore - currentScore + 1);
        auto *tip = new QLabel(QString("当前第 %1 名，离第十名还差 %2 分。")
                                   .arg(currentRank)
                                   .arg(difference));
        tip->setAlignment(Qt::AlignCenter);
        tip->setStyleSheet("font-size:13px;color:#776e65;");
        layout->addWidget(tip);
    }
    else if (!guestMode && currentRank < 0)
    {
        auto *tip = new QLabel("当前分数过低，请继续游戏。");
        tip->setAlignment(Qt::AlignCenter);
        tip->setStyleSheet("font-size:13px;color:#776e65;");
        layout->addWidget(tip);
    }
    showDetailPage("排行榜", content);
}

void MainWindow::showHistory()
{
    if (guestMode)
        return;
    HistoryEntry entries[HISTORY_ENTRIES_MAX];
    const QByteArray username = currentUser.toUtf8();
    const int count = history_load_user(HISTORY_DATA_FILE, username.constData(),
                                        entries, HISTORY_ENTRIES_MAX);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *table = new QTableWidget(count, 7);
    table->setAlternatingRowColors(true);
    table->setHorizontalHeaderLabels(
        {"完成时间", "分数", "最大方块", "步数", "用时", "模式", "用户"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    for (int row = 0; row < count; ++row)
    {
        const HistoryEntry &entry = entries[count - row - 1];
        table->setItem(row, 0, new QTableWidgetItem(QDateTime::fromSecsSinceEpoch(entry.finished_at).toString("yyyy-MM-dd HH:mm")));
        table->setItem(row, 1, new QTableWidgetItem(QString::number(entry.score)));
        table->setItem(row, 2, new QTableWidgetItem(QString::number(entry.max_tile)));
        table->setItem(row, 3, new QTableWidgetItem(QString::number(entry.steps)));
        table->setItem(row, 4, new QTableWidgetItem(formatTime(entry.elapsed_seconds)));
        table->setItem(row, 5, new QTableWidgetItem(modeDisplayName(entry.mode)));
        table->setItem(row, 6, new QTableWidgetItem(QString::fromUtf8(entry.username)));
    }
    layout->addWidget(table);
    showDetailPage("我的历史成绩", content);
}

void MainWindow::showUserCenter()
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
    oldPassword->setPlaceholderText("请输入当前密码");
    newPassword->setPlaceholderText("请输入6-20位新密码");
    confirmation->setPlaceholderText("请再次输入新密码");
    form->addRow("当前用户", new QLabel(currentUser));
    form->addRow("当前密码", oldPassword);
    form->addRow("新密码", newPassword);
    form->addRow("确认新密码", confirmation);
    layout->addLayout(form);
    auto *changeButton = makeButton("修改密码");
    auto *clearButton = makeButton("清理存档、排行和历史数据");
    auto *deleteButton = makeButton("删除账号");
    auto *message = new QLabel;
    message->setWordWrap(true);
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
                    QMessageBox::information(this, "修改密码",
                                             "密码修改成功，请重新登录。");
                    logout();
                    return;
                }
                message->setText(authMessage(result));
            });
    connect(clearButton, &QPushButton::clicked, content,
            [this, message]
            {
                if (QMessageBox::question(this, "清理数据",
                                          "将删除当前账号的存档、排行榜记录和历史成绩，账号会保留。是否继续？") != QMessageBox::Yes)
                    return;
                const QByteArray user = currentUser.toUtf8();
                (void)storage_delete_save(SAVES_DATA_FILE, user.constData());
                const bool rankOk = rank_delete_user(SCORES_DATA_FILE, user.constData());
                const bool historyOk = history_delete_user(HISTORY_DATA_FILE, user.constData());
                message->setText(rankOk && historyOk ? "用户数据已清理。" : "部分数据清理失败。");
            });
    connect(deleteButton, &QPushButton::clicked, content,
            [this, oldPassword, message]
            {
                if (oldPassword->text().isEmpty())
                {
                    message->setText("删除账号前请输入当前密码。");
                    return;
                }
                if (QMessageBox::warning(this, "删除账号",
                                         "账号及其全部数据将永久删除，是否继续？",
                                         QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
                    return;
                const QByteArray user = currentUser.toUtf8();
                const QByteArray password = oldPassword->text().toUtf8();
                const AuthResult result = auth_delete_user(
                    USER_DATA_FILE, user.constData(), password.constData());
                if (result != AUTH_OK)
                {
                    message->setText(authMessage(result));
                    return;
                }
                (void)storage_delete_save(SAVES_DATA_FILE, user.constData());
                (void)rank_delete_user(SCORES_DATA_FILE, user.constData());
                (void)history_delete_user(HISTORY_DATA_FILE, user.constData());
                QMessageBox::information(this, "删除账号", "账号已删除。");
                logout();
            });
    showDetailPage("用户中心", content);
}

void MainWindow::showSettings()
{
    QSettings settings(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/settings.ini",
                       QSettings::IniFormat);
    auto *content = new QWidget;
    auto *layout = new QVBoxLayout(content);
    auto *form = new QFormLayout;
    auto *interval = new QSpinBox;
    interval->setRange(0, 100);
    interval->setSpecialValueText("关闭自动保存");
    interval->setSuffix(" 步");
    interval->setValue(settings.value("autoSaveInterval", 5).toInt());
    form->addRow("有效移动后自动保存", interval);
    layout->addLayout(form);
    auto *modInfo = new QLabel(QString(
                                   "玩法扩展接口已预留，可注册计步、限时及自定义棋盘模式。\n"
                                   "MOD 目录：%1/Mods\n当前已注册模式：%2")
                                   .arg(QString::fromUtf8(GAME_DATA_DIRECTORY))
                                   .arg(game_mode_count()));
    modInfo->setWordWrap(true);
    layout->addWidget(modInfo);
    auto *message = new QLabel;
    auto *saveSettingsButton = makeButton("保存设置");
    layout->addWidget(message);
    layout->addWidget(saveSettingsButton);
    layout->addStretch();
    connect(saveSettingsButton, &QPushButton::clicked, content,
            [interval, message]
            {
                QSettings settings(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/settings.ini",
                                   QSettings::IniFormat);
                settings.setValue("autoSaveInterval", interval->value());
                message->setText("设置已保存。");
            });
    showDetailPage("设置", content);
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
        "自动保存：默认每 5 次有效移动保存，可在设置中修改或关闭\n\n"
        "模式接口：核心层支持注册新模式，后续可扩展计步、限时和自定义棋盘玩法");
    help->setWordWrap(true);
    help->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    help->setStyleSheet("font-size:16px;line-height:1.5;");
    layout->addWidget(help);
    layout->addStretch();
    showDetailPage("帮助", content);
}

void MainWindow::renderBoard()
{
    updateGameInfo();
    boardWidget->setBoard(board);
    undoButton->setEnabled(canUndo);
    saveButton->setEnabled(!guestMode);
    gameMessage->setText(guestMode
                             ? "游客模式 · 方向键或 W/A/S/D 移动方块"
                             : "方向键或 W/A/S/D 移动方块");
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
    BoardCommand command;
    switch (event->key())
    {
    case Qt::Key_Up:
    case Qt::Key_W:
        command = BOARD_CMD_UP;
        break;
    case Qt::Key_Down:
    case Qt::Key_S:
        command = BOARD_CMD_DOWN;
        break;
    case Qt::Key_Left:
    case Qt::Key_A:
        command = BOARD_CMD_LEFT;
        break;
    case Qt::Key_Right:
    case Qt::Key_D:
        command = BOARD_CMD_RIGHT;
        break;
    default:
        QMainWindow::keyPressEvent(event);
        return;
    }

    handleMove(command);
}

void MainWindow::handleMove(BoardCommand command)
{
    updateTimeLabel();
    if (boardWidget->isAnimating())
    {
        queuedMove = command;
        hasQueuedMove = true;
        return;
    }

    Board previous = board;
    previous.elapsed_seconds = currentElapsedSeconds();
    if (!board_process(&board, command))
    {
        gameMessage->setText("这个方向无法移动。");
        updateTimeLabel();
        return;
    }
    undoBoard = previous;
    canUndo = true;
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
    const BoardStatus status = board_judge(&board);
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
    bool rankSaved = false;
    bool historySaved = false;
    if (!guestMode)
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
        historySaved = history_save(HISTORY_DATA_FILE, &entry);
        (void)storage_delete_save(SAVES_DATA_FILE, username.constData());
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
    QSettings settings(QString::fromUtf8(GAME_DATA_DIRECTORY) + "/settings.ini",
                       QSettings::IniFormat);
    return settings.value("autoSaveInterval", 5).toInt();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (gameActive && !guestMode && !currentUser.isEmpty())
        saveGame(false);
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

QString MainWindow::modeDisplayName(const char *mode) const
{
    const GameModeDefinition *definition = game_mode_find(
        mode == nullptr || mode[0] == '\0' ? "classic" : mode);
    return definition == nullptr ? QString::fromUtf8(mode)
                                 : QString::fromUtf8(definition->display_name);
}
