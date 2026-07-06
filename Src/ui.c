#include "ui.h"

#include <stdio.h>

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

void ui_show_ranking(void)
{
    ui_clear_screen();
    puts("================================");
    puts("             排行榜              ");
    puts("================================");
    puts("排行榜模块尚未接入。");
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
