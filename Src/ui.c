#include "ui.h"
#include "config.h"
#include "utils.h"

#include <stdio.h>

#ifdef _WIN32
#include <windows.h>

#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
#endif

void ui_initialize_console(void)
{
#ifdef _WIN32
    HANDLE output_handle;
    DWORD output_mode;

    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    if (output_handle != INVALID_HANDLE_VALUE &&
        GetConsoleMode(output_handle, &output_mode))
    {
        SetConsoleMode(output_handle,
                       output_mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
    }
#endif
}

void ui_show_welcome(void)
{
    ui_clear_screen();
    puts("================================");
    puts("        欢迎来到 2048 游戏       ");
    puts("================================");
    puts("      合并数字，挑战最高分！      ");
}

void ui_show_main_menu(const char *current_user)
{
    ui_clear_screen();
    puts("================================");
    puts("          2048 游戏系统          ");
    puts("================================");

    if (current_user != NULL && current_user[0] != '\0')
    {
        printf("当前登录用户：%s\n", current_user);
    }
    else
    {
        puts("当前状态：未登录");
    }

    puts("--------------------------------");
    puts("1. 用户中心（登录 / 注册）");
    puts("2. 开始游戏");
    puts("3. 查看排行榜");
    puts("0. 退出程序");
    puts("================================");
}

void ui_show_user_menu(const char *current_user)
{
    ui_clear_screen();
    puts("========== 用户中心 ==========");

    if (current_user != NULL && current_user[0] != '\0')
    {
        printf("当前用户：%s\n", current_user);
        puts("1. 注销登录");
        puts("0. 返回主界面");
    }
    else
    {
        puts("1. 登录");
        puts("2. 注册");
        puts("0. 返回主界面");
    }
}

void ui_show_register_page(void)
{
    ui_clear_screen();
    puts("========== 用户注册 ==========");
}

void ui_show_login_page(void)
{
    ui_clear_screen();
    puts("========== 用户登录 ==========");
}

void ui_show_game(const char *current_user)
{
    ui_clear_screen();
    puts("================================");
    puts("            2048 游戏            ");
    puts("================================");
    printf("玩家：%s\n", current_user);
    puts("游戏模块尚未接入，下一步将在这里绘制棋盘。");
}

static const char *ui_tile_color(int value)
{
    switch (value)
    {
    case 0: return "\033[90;100m";
    case 2: return "\033[30;107m";
    case 4: return "\033[30;47m";
    case 8: return "\033[97;43m";
    case 16: return "\033[97;101m";
    case 32: return "\033[97;41m";
    case 64: return "\033[97;45m";
    case 128: return "\033[30;103m";
    case 256: return "\033[30;93m";
    case 512: return "\033[97;33m";
    case 1024: return "\033[97;35m";
    default: return "\033[97;44m";
    }
}

static void ui_show_board(const Board *board)
{
    int row;
    int col;

    for (row = 0; row < BOARD_ROWS; row = -~row)
    {
        puts("+-------+-------+-------+-------+");
        for (col = 0; col < BOARD_COLS; col = -~col)
        {
            if (board->grid[row][col] == 0)
                printf("| %s%*s\033[0m ", ui_tile_color(0), BOARD_CELL_WIDTH, "");
            else
                printf("| %s%*d\033[0m ",
                       ui_tile_color(board->grid[row][col]),
                       BOARD_CELL_WIDTH,
                       board->grid[row][col]);
        }
        puts("|");
    }
    puts("+-------+-------+-------+-------+");
}

void ui_show_game_state(const Board *board, const char *current_user)
{
    ui_clear_screen();
    puts("================================");
    puts("            2048 游戏           ");
    puts("================================");
    printf("玩家：%s    分数：%d    步数：%d\n",
           current_user,
           board->score,
           board->step);
    ui_show_board(board);
    puts("方向键/WASD：移动  P：保存  Q：返回主菜单");
}

bool ui_ask_continue_save(int saved_score)
{
    char choice;

    while (true)
    {
        ui_clear_screen();
        puts("检测到该用户的游戏存档。");
        printf("存档分数：%d\n", saved_score);
        puts("1. 继续游戏");
        puts("2. 重新开始");

        if (utils_read_choice(&choice) && (choice == '1' || choice == '2'))
            return choice == '1';
    }
}

void ui_show_quit_save_result(bool success)
{
    puts(success ? "游戏进度已自动保存。" : "游戏进度自动保存失败。");
}

void ui_show_game_message(const char *message)
{
    if (message != NULL)
        printf("提示：%s\n", message);
}

void ui_show_game_over(const Board *board,
                       BoardStatus status,
                       bool rank_saved)
{
    ui_clear_screen();
    puts(status == BOARD_STATUS_WIN ? "恭喜你合成了 2048！" : "没有可移动方块，游戏结束。");
    printf("最终分数：%d，移动步数：%d\n", board->score, board->step);
    ui_show_board(board);
    puts(rank_saved ? "成绩已更新到排行榜。" : "排行榜保存失败。");
}

void ui_show_ranking(const RankEntry *entries, int count, const char *current_user, int user_rank, int user_best)
{
    int display_count;

    ui_clear_screen();
    puts("================================");
    puts("             排行榜              ");
    puts("================================");

    if (count == 0)
    {
        puts("");
        puts("           暂无排行数据           ");
        return;
    }

    display_count = (count < RANK_TOP_COUNT) ? count : RANK_TOP_COUNT;

    printf("\n%-6s %-20s %s\n", "排名", "用户名", "分数");
    puts("--------------------------------");

    for (int i = 0; i < display_count; i = -~i)
        printf("%-6d %-20s %d\n", i + 1, entries[i].username, entries[i].score);

    puts("--------------------------------");

    if (current_user == NULL || current_user[0] == '\0')
    {
        puts("登录后可查看您的个人排名。");
        return;
    }

    if (user_rank < 0)
    {
        puts("您暂无游戏记录或者分数太低，快去开始游戏吧！");
    }
    else if (user_rank < RANK_TOP_COUNT)
    {
        printf("恭喜您所处排行榜第%d名！\n", user_rank + 1);
    }
    else
    {
        int tenth_score = entries[RANK_TOP_COUNT - 1].score;
        printf("您的最佳分数为%d分，距离排行榜第十名还有%d分差距！\n", user_best, tenth_score - user_best);
    }
}

void ui_clear_screen(void)
{
    printf("\033[2J\033[H");
}

void ui_wait_for_enter(void)
{
    int ch;

    puts("\n按回车键继续...");

    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
}
