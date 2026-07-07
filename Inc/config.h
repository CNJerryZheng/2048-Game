#pragma once
#define USER_DATA_FILE "Data/user.dat"
#define SCORES_DATA_FILE "Data/scores.dat"
#define SAVES_DATA_FILE "Data/saves.dat"
#define HISTORY_DATA_FILE "Data/history.dat"

#define USER_PASSWORD_LENGTH_MAX 21 // 最长密码长度20+冗余位置1
#define USER_PASSWORD_LENGTH_MIN 6  // 最短密码长度

#define USER_NAME_LENGTH_MAX 32 // 最长用户名长度31+冗余位置1
#define USER_NAME_LENGTH_MIN 2  // 最短用户名长度

#define RANK_ENTRIES_MAX 256 // 最多读取的排行榜记录数
#define RANK_TOP_COUNT 10    // 排行榜展示前十名

#define INPUT_BUFFER_LENGTH 128 // 输入缓冲区长度

#define BOARD_CELL_WIDTH 5             // 控制台棋盘单元格宽度
#define GAME_NEW_TILE_TWO_PERCENT 90   // 新方块生成数字2的概率（百分比）
#define SAVE_ENTRIES_MAX 256           // 最多保存的用户存档数

#define HASH_BASE1 131u       // 第一组多项式哈希底数
#define HASH_BASE2 13331u     // 第二组多项式哈希底数
#define HASH_MOD1 1000000007u // 第一组哈希取模质数
#define HASH_MOD2 1000000009u // 第二组哈希取模质数
