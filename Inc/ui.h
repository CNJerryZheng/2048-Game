#pragma once

#include <stdbool.h>

#include "board.h"
#include "rank.h"

void ui_initialize_console(void); // 初始化控制台，设置编码为UTF-8，隐藏光标

void ui_show_welcome(void); // 显示欢迎界面

void ui_show_main_menu(const char *current_user); // 显示主菜单，current_user为当前登录的用户名，如果未登录则为NULL或空字符串

void ui_show_user_menu(const char *current_user); // 显示用户菜单，current_user为当前登录的用户名，如果未登录则为NULL或空字符串

void ui_show_register_page(void); // 显示注册界面

void ui_show_login_page(void); // 显示登录界面

void ui_show_game(const char *current_user); // 显示游戏界面，current_user为当前登录的用户名，如果未登录则为NULL或空字符串

void ui_show_game_state(const Board *board, const char *current_user);
bool ui_ask_continue_save(int saved_score);
void ui_show_quit_save_result(bool success);
void ui_show_game_message(const char *message);
void ui_show_game_over(const Board *board,
                       BoardStatus status,
                       bool rank_saved);

void ui_show_ranking(const RankEntry *entries, int count, const char *current_user, int user_rank, int user_best); // 显示排行榜界面

void ui_clear_screen(void); // 清屏

void ui_wait_for_enter(void); // 等待用户按回车键
