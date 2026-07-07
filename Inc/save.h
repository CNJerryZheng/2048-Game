#pragma once

#include <stdbool.h>

#include "board.h"
#include "config.h"

typedef struct
{
    char username[USER_NAME_LENGTH_MAX];
    int score;
    bool won;
    int tiles[BOARD_SIZE][BOARD_SIZE];
} SaveRecord;

// 从存档文件中加载指定用户的存档。
// 找到则把数据填入 out 指向的 Board（同时把 score/won 同步过去），返回 true；
// 未找到或解析失败返回 false。
bool save_load(const char *saves_file, const char *username, Board *out);

// 把当前对局（board 内的棋盘与分数）写入指定用户的存档；
// 若用户已有存档则覆盖，否则追加。成功返回 true。
bool save_persist(const char *saves_file, const char *username, const Board *board);

// 删除指定用户的存档（例如重新开局时调用）。成功返回 true。
bool save_delete(const char *saves_file, const char *username);

// 判断指定用户是否存在有效存档（用于决定主菜单是否显示"继续上局"）。
bool save_exists(const char *saves_file, const char *username);
