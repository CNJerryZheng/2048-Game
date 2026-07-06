#include <stdio.h>
#include <string.h>
#include <stdbool.h>

#include "user.h"
#include "config.h"
#include "ui.h"
#include "utils.h"

static void run_game_system(void)
{
    char choice;
    char current_user[USER_NAME_LENGTH_MAX] = "";

    ui_show_welcome();
    ui_wait_for_enter();

    while (true)
    {
        ui_show_main_menu(current_user);

        if (!utils_read_choice(&choice))
        {
            puts("输入错误，请重新输入!");
            ui_wait_for_enter();
            continue;
        }

        if (choice == '1')
        {
            user_show_interface(USER_DATA_FILE,
                                current_user,
                                sizeof(current_user));
        }
        else if (choice == '2')
        {
            if (current_user[0] == '\0')
            {
                puts("请先登录后再开始游戏！");
            }
            else
            {
                ui_show_game(current_user);
            }
            ui_wait_for_enter();
        }
        else if (choice == '3')
        {
            ui_show_ranking();
            ui_wait_for_enter();
        }
        else if (choice == '0')
        {
            puts("感谢您的游玩，再见！");
            break;
        }
        else
        {
            puts("无效选项，请重新输入。 ");
            ui_wait_for_enter();
            continue;
        }
    }

    ui_wait_for_enter();
}

int main(void)
{
    run_game_system();

    return 0;
}
