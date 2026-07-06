#include "ui.h"

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
