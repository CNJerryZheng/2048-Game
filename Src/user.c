#include "user.h"

#include <ctype.h>
#include <stdio.h>
#include <string.h>

#define USER_PASSWORD_LENGTH 64
#define INPUT_BUFFER_LENGTH 128

static int read_line(const char *prompt, char *buffer, size_t size)
{
    int character;
    size_t length;

    if (prompt != NULL) {
        fputs(prompt, stdout);
        fflush(stdout);
    }

    if (fgets(buffer, (int)size, stdin) == NULL) {
        return 0;
    }

    length = strlen(buffer);
    if (length > 0 && buffer[length - 1] == '\n') {
        buffer[length - 1] = '\0';
        if (length > 1 && buffer[length - 2] == '\r') {
            buffer[length - 2] = '\0';
        }
    } else {
        while ((character = getchar()) != '\n' && character != EOF) {
        }
    }

    return 1;
}

static void wait_for_enter(void)
{
    char input[INPUT_BUFFER_LENGTH];
    (void)read_line("\n按回车键继续...", input, sizeof(input));
}

static int username_is_valid(const char *username)
{
    size_t index;
    size_t length = strlen(username);

    if (length < 3 || length >= USER_NAME_LENGTH) {
        return 0;
    }

    for (index = 0; index < length; ++index) {
        unsigned char character = (unsigned char)username[index];
        if (!isalnum(character) && character != '_') {
            return 0;
        }
    }

    return 1;
}

static int password_is_valid(const char *password)
{
    size_t index;
    size_t length = strlen(password);

    if (length < 6 || length >= USER_PASSWORD_LENGTH) {
        return 0;
    }

    for (index = 0; index < length; ++index) {
        if (isspace((unsigned char)password[index])) {
            return 0;
        }
    }

    return 1;
}

static int read_user_record(FILE *file,
                            char *username,
                            size_t username_size,
                            char *password,
                            size_t password_size)
{
    char line[INPUT_BUFFER_LENGTH];
    char stored_username[USER_NAME_LENGTH];
    char stored_password[USER_PASSWORD_LENGTH];

    while (fgets(line, sizeof(line), file) != NULL) {
        if (sscanf(line, "%31[^\t]\t%63[^\r\n]",
                   stored_username, stored_password) == 2) {
            strncpy(username, stored_username, username_size - 1);
            username[username_size - 1] = '\0';
            strncpy(password, stored_password, password_size - 1);
            password[password_size - 1] = '\0';
            return 1;
        }
    }

    return 0;
}

static int username_exists(const char *user_file, const char *username)
{
    FILE *file = fopen(user_file, "r");
    char stored_username[USER_NAME_LENGTH];
    char stored_password[USER_PASSWORD_LENGTH];

    if (file == NULL) {
        return 0;
    }

    while (read_user_record(file,
                            stored_username, sizeof(stored_username),
                            stored_password, sizeof(stored_password))) {
        if (strcmp(username, stored_username) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

static int register_user(const char *user_file)
{
    FILE *file;
    char username[USER_NAME_LENGTH];
    char password[USER_PASSWORD_LENGTH];
    char confirmation[USER_PASSWORD_LENGTH];

    puts("\n========== 用户注册 ==========");
    if (!read_line("请输入用户名（3-31位字母、数字或下划线）：",
                   username, sizeof(username))) {
        return 0;
    }

    if (!username_is_valid(username)) {
        puts("用户名格式不正确。");
        return 0;
    }

    if (username_exists(user_file, username)) {
        puts("该用户名已被注册。");
        return 0;
    }

    if (!read_line("请输入密码（6-63位，不能包含空格）：",
                   password, sizeof(password)) ||
        !password_is_valid(password)) {
        puts("密码格式不正确。");
        return 0;
    }

    if (!read_line("请再次输入密码：", confirmation, sizeof(confirmation))) {
        return 0;
    }

    if (strcmp(password, confirmation) != 0) {
        puts("两次输入的密码不一致。");
        return 0;
    }

    file = fopen(user_file, "a");
    if (file == NULL) {
        printf("无法打开用户数据文件：%s\n", user_file);
        return 0;
    }

    if (fprintf(file, "%s\t%s\n", username, password) < 0 || fclose(file) != 0) {
        puts("保存注册信息失败。");
        return 0;
    }

    puts("注册成功，现在可以登录。 ");
    return 1;
}

static int login_user(const char *user_file,
                      char *current_user,
                      size_t current_user_size)
{
    FILE *file;
    char username[USER_NAME_LENGTH];
    char password[USER_PASSWORD_LENGTH];
    char stored_username[USER_NAME_LENGTH];
    char stored_password[USER_PASSWORD_LENGTH];

    puts("\n========== 用户登录 ==========");
    if (!read_line("用户名：", username, sizeof(username)) ||
        !read_line("密码：", password, sizeof(password))) {
        return 0;
    }

    file = fopen(user_file, "r");
    if (file == NULL) {
        puts("暂无用户数据，请先注册。 ");
        return 0;
    }

    while (read_user_record(file,
                            stored_username, sizeof(stored_username),
                            stored_password, sizeof(stored_password))) {
        if (strcmp(username, stored_username) == 0 &&
            strcmp(password, stored_password) == 0) {
            strncpy(current_user, username, current_user_size - 1);
            current_user[current_user_size - 1] = '\0';
            fclose(file);
            printf("登录成功，欢迎你：%s！\n", current_user);
            return 1;
        }
    }

    fclose(file);
    puts("用户名或密码错误。 ");
    return 0;
}

void user_show_interface(const char *user_file,
                         char *current_user,
                         size_t current_user_size)
{
    char choice[INPUT_BUFFER_LENGTH];

    for (;;) {
        puts("\n========== 用户中心 ==========");
        if (current_user[0] != '\0') {
            printf("当前用户：%s\n", current_user);
            puts("1. 注销登录");
            puts("0. 返回主界面");
            if (!read_line("请选择：", choice, sizeof(choice))) {
                return;
            }

            if (strcmp(choice, "1") == 0) {
                current_user[0] = '\0';
                puts("已注销登录。 ");
                wait_for_enter();
            } else if (strcmp(choice, "0") == 0) {
                return;
            } else {
                puts("无效选项，请重新输入。 ");
            }
        } else {
            puts("1. 登录");
            puts("2. 注册");
            puts("0. 返回主界面");
            if (!read_line("请选择：", choice, sizeof(choice))) {
                return;
            }

            if (strcmp(choice, "1") == 0) {
                (void)login_user(user_file, current_user, current_user_size);
                wait_for_enter();
            } else if (strcmp(choice, "2") == 0) {
                (void)register_user(user_file);
                wait_for_enter();
            } else if (strcmp(choice, "0") == 0) {
                return;
            } else {
                puts("无效选项，请重新输入。 ");
            }
        }
    }
}
