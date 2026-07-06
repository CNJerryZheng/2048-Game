#pragma once
#define USER_DATA_FILE "Data/user.txt"
#define SCORES_DATA_FILE "Data/scores.txt"

#define USER_PASSWORD_LENGTH_MAX 21 // 最长密码长度20+冗余位置1
#define USER_PASSWORD_LENGTH_MIN 6  // 最短密码长度

#define USER_NAME_LENGTH_MAX 32 // 最长用户名长度31+冗余位置1
#define USER_NAME_LENGTH_MIN 2  // 最短用户名长度

#define INPUT_BUFFER_LENGTH 128 // 输入缓冲区长度

#define RANK_ENTRIES_MAX 256 // 最多读取的排行榜记录数
#define RANK_TOP_COUNT 10    // 排行榜展示前十名
