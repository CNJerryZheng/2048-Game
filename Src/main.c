#include <stdio.h>
#include <string.h>

#ifdef _WIN32
#include <windows.h>
#endif

#include "user.h"
#include "config.h"

static int read_choice(char *buffer, size_t size)
{
    int character;
    size_t length;

    fputs("请选择：", stdout);
    fflush(stdout);

    if (fgets(buffer, (int)size, stdin) == NULL)
    {
        return 0;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n')
    {
        buffer[length - 1] = '\0';
    }
    else
    {
        while ((character = getchar()) != '\n' && character != EOF)
        {
        }
    }

    return 1;
}

int main(void)
{
#ifdef _WIN32
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
#endif

    char choice[16];
    char current_user[USER_NAME_LENGTH] = "";

    for (;;)
    {
        puts("\n================================");
        puts("          2048 游戏系统");
        puts("================================");
        if (current_user[0] != '\0')
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
        puts("0. 退出程序");
        puts("================================");

        if (!read_choice(choice, sizeof(choice)))
        {
            break;
        }

        if (strcmp(choice, "1") == 0)
        {
            user_show_interface(USER_DATA_FILE,
                                current_user,
                                sizeof(current_user));
        }
        else if (strcmp(choice, "2") == 0)
        {
            if (current_user[0] == '\0')
            {
                puts("请先登录后再开始游戏。 ");
            }
            else
            {
                puts("游戏模块尚未接入，下一步将在这里启动 2048。 ");
            }
        }
        else if (strcmp(choice, "0") == 0)
        {
            puts("感谢使用，再见！");
            break;
        }
        else
        {
            puts("无效选项，请重新输入。 ");
        }
    }

    return 0;
}
