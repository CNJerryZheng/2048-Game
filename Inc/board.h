#pragma once

#include <stdbool.h>
#include "config.h"

#define BOARD_ROWS 4
#define BOARD_COLS 4
#define BOARD_TARGET 2048

typedef enum BoardCommand
{
    BOARD_CMD_UP,
    BOARD_CMD_DOWN,
    BOARD_CMD_LEFT,
    BOARD_CMD_RIGHT
} BoardCommand;

typedef enum BoardStatus
{
    BOARD_STATUS_WAIT,
    BOARD_STATUS_PROCESS,
    BOARD_STATUS_WIN,
    BOARD_STATUS_LOSE
} BoardStatus;

typedef struct Board
{
    bool game_start;
    bool game_over;
    int grid[BOARD_ROWS][BOARD_COLS];
    int score;
    int step;
    int elapsed_seconds;
    char mode[GAME_MODE_ID_LENGTH];
} Board;

#ifdef __cplusplus
extern "C" {
#endif

void board_init(Board *board);
void board_start(Board *board);
bool board_create_number(Board *board);
bool board_process(Board *board, BoardCommand command);
BoardStatus board_judge(const Board *board);
int board_get_data(const Board *board, int row, int col);
int board_get_score(const Board *board);
int board_get_step(const Board *board);
bool board_has_started(const Board *board);
bool board_is_over(const Board *board);
int board_get_max_tile(const Board *board);

#ifdef __cplusplus
}
#endif
