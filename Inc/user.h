#pragma once

#include <stddef.h>

/*
 * 显示用户中心。
 * 登录成功后会把用户名写入 current_user，注销后会将其清空。
 */
void user_show_interface(const char *user_file, char *current_user, size_t current_user_size);
