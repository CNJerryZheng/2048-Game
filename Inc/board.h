#pragma once

#include <stdbool.h>

#include "config.h"

typedef struct
{
    int tiles[BOARD_SIZE][BOARD_SIZE]; // 棋盘格子，0 表示空
    int score;                         // 当前对局分数
    bool won;                          // 是否曾经达到 2048
} Board;

// 方向枚举
typedef enum
{
    BOARD_DIR_UP = 0,
    BOARD_DIR_DOWN,
    BOARD_DIR_LEFT,
    BOARD_DIR_RIGHT
} BoardDirection;

// 移动结果
typedef enum
{
    BOARD_MOVE_NO_CHANGE = 0, // 没有可移动的方块
    BOARD_MOVE_OK,            // 移动成功（可能产生合并）
    BOARD_MOVE_WIN            // 移动成功并触发 2048 胜利
} BoardMoveResult;

void board_initialize(Board *board);                 // 初始化棋盘：清空分数与方块，并随机生成两个起始方块
void board_render(const Board *board);               // 渲染棋盘到控制台
bool board_spawn_tile(Board *board);                 // 在随机空格上生成新方块（90% 为 2，10% 为 4），返回是否成功
BoardMoveResult board_move(Board *board, BoardDirection direction); // 按指定方向执行一次移动与合并
bool board_is_game_over(const Board *board);         // 判断游戏是否结束（无空格且无相邻可合并方块）
void board_get_max_tile(const Board *board, int *max_tile); // 获取棋盘上的最大方块数值
