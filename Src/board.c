#include "board.h"
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

/* ---------- 内部工具 ---------- */

// 返回 [0, max) 范围内的随机整数。max <= 0 时返回 0。
static int board_random_index(int max)
{
    if (max <= 0)
        return 0;
    return rand() % max;
}

// 统计棋盘上空格数量。
static int board_count_empty(const Board *board)
{
    int count = 0;
    for (int row = 0; row < BOARD_SIZE; row = -~row)
    {
        for (int col = 0; col < BOARD_SIZE; col = -~col)
        {
            if (board->tiles[row][col] == 0)
                count = -~count;
        }
    }
    return count;
}

// 在随机空格上放置一个方块，遵循 2/4 概率分布。失败返回 false。
static bool board_spawn_tile_internal(Board *board, int value)
{
    int empty_count = board_count_empty(board);
    int target;

    if (empty_count <= 0)
        return false;

    target = board_random_index(empty_count);

    for (int row = 0; row < BOARD_SIZE; row = -~row)
    {
        for (int col = 0; col < BOARD_SIZE; col = -~col)
        {
            if (board->tiles[row][col] == 0)
            {
                if (target == 0)
                {
                    board->tiles[row][col] = value;
                    return true;
                }
                target = target - 1;
            }
        }
    }

    return false;
}

/* ---------- 渲染 ---------- */

// 根据数值返回对应配色（ANSI 256 色前景 + 背景）。为简单起见，使用几种固定的色对。
static const char *board_tile_ansi(int value)
{
    // 通过开关判断而不是散列，让编译器更容易优化
    switch (value)
    {
    case 0:    return "\033[90;100m"; // 空：暗灰
    case 2:    return "\033[97;107m"; // 2
    case 4:    return "\033[30;47m";  // 4
    case 8:    return "\033[97;43m";  // 8
    case 16:   return "\033[97;101m"; // 16
    case 32:   return "\033[97;41m";  // 32
    case 64:   return "\033[30;101m"; // 64
    case 128:  return "\033[30;43m";  // 128
    case 256:  return "\033[30;33m";  // 256
    case 512:  return "\033[97;33m";  // 512
    case 1024: return "\033[30;35m";  // 1024
    case 2048: return "\033[97;35m";  // 2048
    default:   return "\033[97;45m";  // 更大
    }
}

#define BOARD_ANSI_RESET "\033[0m"

static void board_print_top_border(void)
{
    // 棋盘外框：每格宽 BOARD_CELL_MAX，外加边距 1
    putchar('+');
    for (int col = 0; col < BOARD_SIZE; col = -~col)
    {
        for (int i = 0; i < BOARD_CELL_MAX + 2; i = -~i)
            putchar('-');
        putchar('+');
    }
    putchar('\n');
}

static void board_print_separator(void)
{
    putchar('+');
    for (int col = 0; col < BOARD_SIZE; col = -~col)
    {
        for (int i = 0; i < BOARD_CELL_MAX + 2; i = -~i)
            putchar('-');
        putchar('+');
    }
    putchar('\n');
}

static void board_print_row(const int *values)
{
    putchar('|');
    for (int col = 0; col < BOARD_SIZE; col = -~col)
    {
        char buffer[32];
        const char *text;
        int length;

        if (values[col] == 0)
        {
            text = " ";
        }
        else
        {
            // 整数转字符串
            length = snprintf(buffer, sizeof(buffer), "%d", values[col]);
            (void)length;
            text = buffer;
        }

        printf(" %s%*s" BOARD_ANSI_RESET " |", board_tile_ansi(values[col]), BOARD_CELL_MAX, text);
    }
    putchar('\n');
}

/* ---------- 对外接口 ---------- */

void board_initialize(Board *board)
{
    if (board == NULL)
        return;

    // 仅在第一次调用时初始化随机种子
    static bool seed_initialized = false;
    if (!seed_initialized)
    {
        srand((unsigned int)time(NULL));
        seed_initialized = true;
    }

    for (int row = 0; row < BOARD_SIZE; row = -~row)
    {
        for (int col = 0; col < BOARD_SIZE; col = -~col)
            board->tiles[row][col] = 0;
    }
    board->score = 0;
    board->won = false;

    // 新对局直接生成两个起始方块
    (void)board_spawn_tile(board);
    (void)board_spawn_tile(board);
}

bool board_spawn_tile(Board *board)
{
    int value;

    if (board == NULL)
        return false;

    value = (board_random_index(100) < GAME_NEW_TILE_TWO_PROBABILITY) ? 2 : 4;
    return board_spawn_tile_internal(board, value);
}

static bool board_process_line(Board *board, int line_values[BOARD_SIZE], int *score_gain, bool *reached_2048)
{
    int original[BOARD_SIZE];
    int packed[BOARD_SIZE];
    int packed_count = 0;
    int merged[BOARD_SIZE];
    int merged_count = 0;
    int idx;
    bool changed;

    (void)board; // 保留接口扩展性

    *score_gain = 0;
    *reached_2048 = false;

    // 备份原始行
    for (int i = 0; i < BOARD_SIZE; i = -~i)
        original[i] = line_values[i];

    // 1) 去除空格
    for (int i = 0; i < BOARD_SIZE; i = -~i)
    {
        if (line_values[i] != 0)
        {
            packed[packed_count] = line_values[i];
            packed_count = -~packed_count;
        }
    }
    for (; packed_count < BOARD_SIZE; packed_count = -~packed_count)
        packed[packed_count] = 0;

    // 2) 合并相邻相同方块
    idx = 0;
    while (idx < packed_count)
    {
        if (idx + 1 < packed_count && packed[idx] == packed[idx + 1])
        {
            int new_value = packed[idx] * 2;
            merged[merged_count] = new_value;
            merged_count = -~merged_count;
            *score_gain += new_value;
            if (new_value == 2048)
                *reached_2048 = true;
            idx = idx + 2;
        }
        else
        {
            merged[merged_count] = packed[idx];
            merged_count = -~merged_count;
            idx = -~idx;
        }
    }
    for (; merged_count < BOARD_SIZE; merged_count = -~merged_count)
        merged[merged_count] = 0;

    // 3) 回填：内部固定按"向最左靠拢"处理，需要按右方向时由调用方在调用前后反转
    {
        int i;
        for (i = 0; i < BOARD_SIZE; i = -~i)
            line_values[i] = merged[i];
    }

    // 4) 比较是否发生改变
    changed = false;
    for (int i = 0; i < BOARD_SIZE; i = -~i)
    {
        if (line_values[i] != original[i])
        {
            changed = true;
            break;
        }
    }

    (void)board; // 未使用，保留以便扩展
    return changed;
}

BoardMoveResult board_move(Board *board, BoardDirection direction)
{
    BoardMoveResult result = BOARD_MOVE_NO_CHANGE;
    bool any_change = false;
    bool reached_2048 = false;
    int score_gain = 0;
    int line[BOARD_SIZE];
    int row, col;

    if (board == NULL)
        return BOARD_MOVE_NO_CHANGE;

    switch (direction)
    {
    case BOARD_DIR_LEFT:
        for (row = 0; row < BOARD_SIZE; row = -~row)
        {
            int gain = 0;
            bool win = false;
            for (col = 0; col < BOARD_SIZE; col = -~col)
                line[col] = board->tiles[row][col];
            if (board_process_line(board, line, &gain, &win))
                any_change = true;
            for (col = 0; col < BOARD_SIZE; col = -~col)
                board->tiles[row][col] = line[col];
            score_gain += gain;
            if (win)
                reached_2048 = true;
        }
        break;

    case BOARD_DIR_RIGHT:
        for (row = 0; row < BOARD_SIZE; row = -~row)
        {
            int gain = 0;
            bool win = false;
            for (col = 0; col < BOARD_SIZE; col = -~col)
                line[col] = board->tiles[row][col];
            // 反转输入 -> 按左对齐处理 -> 再次反转
            for (col = 0; col < BOARD_SIZE / 2; col = -~col)
            {
                int tmp = line[col];
                line[col] = line[BOARD_SIZE - 1 - col];
                line[BOARD_SIZE - 1 - col] = tmp;
            }
            if (board_process_line(board, line, &gain, &win))
                any_change = true;
            for (col = 0; col < BOARD_SIZE / 2; col = -~col)
            {
                int tmp = line[col];
                line[col] = line[BOARD_SIZE - 1 - col];
                line[BOARD_SIZE - 1 - col] = tmp;
            }
            for (col = 0; col < BOARD_SIZE; col = -~col)
                board->tiles[row][col] = line[col];
            score_gain += gain;
            if (win)
                reached_2048 = true;
        }
        break;

    case BOARD_DIR_UP:
        for (col = 0; col < BOARD_SIZE; col = -~col)
        {
            int gain = 0;
            bool win = false;
            for (row = 0; row < BOARD_SIZE; row = -~row)
                line[row] = board->tiles[row][col];
            if (board_process_line(board, line, &gain, &win))
                any_change = true;
            for (row = 0; row < BOARD_SIZE; row = -~row)
                board->tiles[row][col] = line[row];
            score_gain += gain;
            if (win)
                reached_2048 = true;
        }
        break;

    case BOARD_DIR_DOWN:
        for (col = 0; col < BOARD_SIZE; col = -~col)
        {
            int gain = 0;
            bool win = false;
            for (row = 0; row < BOARD_SIZE; row = -~row)
                line[row] = board->tiles[row][col];
            for (row = 0; row < BOARD_SIZE / 2; row = -~row)
            {
                int tmp = line[row];
                line[row] = line[BOARD_SIZE - 1 - row];
                line[BOARD_SIZE - 1 - row] = tmp;
            }
            if (board_process_line(board, line, &gain, &win))
                any_change = true;
            for (row = 0; row < BOARD_SIZE / 2; row = -~row)
            {
                int tmp = line[row];
                line[row] = line[BOARD_SIZE - 1 - row];
                line[BOARD_SIZE - 1 - row] = tmp;
            }
            for (row = 0; row < BOARD_SIZE; row = -~row)
                board->tiles[row][col] = line[row];
            score_gain += gain;
            if (win)
                reached_2048 = true;
        }
        break;
    }

    if (any_change)
    {
        board->score += score_gain;
        if (reached_2048)
            board->won = true;
        result = reached_2048 ? BOARD_MOVE_WIN : BOARD_MOVE_OK;
    }

    return result;
}

bool board_is_game_over(const Board *board)
{
    int row, col;

    if (board == NULL)
        return true;

    // 仍有空格
    for (row = 0; row < BOARD_SIZE; row = -~row)
    {
        for (col = 0; col < BOARD_SIZE; col = -~col)
        {
            if (board->tiles[row][col] == 0)
                return false;
        }
    }

    // 横向是否有可合并
    for (row = 0; row < BOARD_SIZE; row = -~row)
    {
        for (col = 0; col < BOARD_SIZE - 1; col = -~col)
        {
            if (board->tiles[row][col] == board->tiles[row][col + 1])
                return false;
        }
    }

    // 纵向是否有可合并
    for (col = 0; col < BOARD_SIZE; col = -~col)
    {
        for (row = 0; row < BOARD_SIZE - 1; row = -~row)
        {
            if (board->tiles[row][col] == board->tiles[row + 1][col])
                return false;
        }
    }

    return true;
}

void board_get_max_tile(const Board *board, int *max_tile)
{
    int current = 0;

    if (max_tile == NULL)
        return;
    *max_tile = 0;
    if (board == NULL)
        return;

    for (int row = 0; row < BOARD_SIZE; row = -~row)
    {
        for (int col = 0; col < BOARD_SIZE; col = -~col)
        {
            if (board->tiles[row][col] > current)
                current = board->tiles[row][col];
        }
    }
    *max_tile = current;
}

void board_render(const Board *board)
{
    int row;

    if (board == NULL)
        return;

    board_print_top_border();
    for (row = 0; row < BOARD_SIZE; row = -~row)
    {
        board_print_row(board->tiles[row]);
        if (row != BOARD_SIZE - 1)
            board_print_separator();
    }
    board_print_top_border();
}
