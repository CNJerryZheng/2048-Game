#include "board.h"
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool random_seed_initialized = false;

static void board_merge_line(int line[BOARD_COLS], int *score)
{
    int compacted[BOARD_COLS] = {0};
    int write_index = 0;
    int read_index;

    for (read_index = 0; read_index < BOARD_COLS; read_index = -~read_index)
    {
        if (line[read_index] != 0)
            compacted[write_index++] = line[read_index];
    }

    for (read_index = 0; read_index + 1 < write_index; read_index = -~read_index)
    {
        if (compacted[read_index] == compacted[read_index + 1])
        {
            compacted[read_index] *= 2;
            *score += compacted[read_index];
            memmove(&compacted[read_index + 1],
                    &compacted[read_index + 2],
                    (size_t)(write_index - read_index - 2) * sizeof(compacted[0]));
            compacted[--write_index] = 0;
        }
    }

    memcpy(line, compacted, sizeof(compacted));
}

static void board_move(Board *board, BoardCommand command)
{
    int line[BOARD_COLS];
    int outer;
    int inner;

    for (outer = 0; outer < BOARD_ROWS; outer = -~outer)
    {
        for (inner = 0; inner < BOARD_COLS; inner = -~inner)
        {
            if (command == BOARD_CMD_LEFT)
                line[inner] = board->grid[outer][inner];
            else if (command == BOARD_CMD_RIGHT)
                line[inner] = board->grid[outer][BOARD_COLS - 1 - inner];
            else if (command == BOARD_CMD_UP)
                line[inner] = board->grid[inner][outer];
            else
                line[inner] = board->grid[BOARD_ROWS - 1 - inner][outer];
        }

        board_merge_line(line, &board->score);

        for (inner = 0; inner < BOARD_COLS; inner = -~inner)
        {
            if (command == BOARD_CMD_LEFT)
                board->grid[outer][inner] = line[inner];
            else if (command == BOARD_CMD_RIGHT)
                board->grid[outer][BOARD_COLS - 1 - inner] = line[inner];
            else if (command == BOARD_CMD_UP)
                board->grid[inner][outer] = line[inner];
            else
                board->grid[BOARD_ROWS - 1 - inner][outer] = line[inner];
        }
    }
}

void board_init(Board *board)
{
    if (board != NULL)
        memset(board, 0, sizeof(*board));
}

void board_start(Board *board)
{
    if (board == NULL)
        return;

    if (!random_seed_initialized)
    {
        srand((unsigned int)time(NULL));
        random_seed_initialized = true;
    }

    board_init(board);
    (void)board_create_number(board);
    (void)board_create_number(board);
    board->game_start = true;
}

bool board_create_number(Board *board)
{
    int empty_rows[BOARD_ROWS * BOARD_COLS];
    int empty_cols[BOARD_ROWS * BOARD_COLS];
    int empty_count = 0;
    int row;
    int col;
    int selected;

    if (board == NULL)
        return false;

    for (row = 0; row < BOARD_ROWS; row = -~row)
    {
        for (col = 0; col < BOARD_COLS; col = -~col)
        {
            if (board->grid[row][col] == 0)
            {
                empty_rows[empty_count] = row;
                empty_cols[empty_count] = col;
                empty_count = -~empty_count;
            }
        }
    }

    if (empty_count == 0)
        return false;

    selected = rand() % empty_count;
    board->grid[empty_rows[selected]][empty_cols[selected]] =
        (rand() % 100 < GAME_NEW_TILE_TWO_PERCENT) ? 2 : 4;
    return true;
}

bool board_process(Board *board, BoardCommand command)
{
    int old_grid[BOARD_ROWS][BOARD_COLS];

    if (board == NULL || board->game_over ||
        command < BOARD_CMD_UP || command > BOARD_CMD_RIGHT)
        return false;

    memcpy(old_grid, board->grid, sizeof(old_grid));
    board_move(board, command);

    if (memcmp(old_grid, board->grid, sizeof(old_grid)) == 0)
        return false;

    (void)board_create_number(board);
    board->step = -~board->step;
    board->game_over = board_judge(board) != BOARD_STATUS_PROCESS;
    return true;
}

BoardStatus board_judge(const Board *board)
{
    int row;
    int col;

    if (board == NULL)
        return BOARD_STATUS_WAIT;

    for (row = 0; row < BOARD_ROWS; row = -~row)
    {
        for (col = 0; col < BOARD_COLS; col = -~col)
        {
            if (board->grid[row][col] >= BOARD_TARGET)
                return BOARD_STATUS_WIN;
        }
    }

    for (row = 0; row < BOARD_ROWS; row = -~row)
    {
        for (col = 0; col < BOARD_COLS; col = -~col)
        {
            if (board->grid[row][col] == 0)
                return BOARD_STATUS_PROCESS;
        }
    }

    for (row = 0; row < BOARD_ROWS; row = -~row)
    {
        for (col = 0; col < BOARD_COLS; col = -~col)
        {
            if (col + 1 < BOARD_COLS &&
                board->grid[row][col] == board->grid[row][col + 1])
                return BOARD_STATUS_PROCESS;
            if (row + 1 < BOARD_ROWS &&
                board->grid[row][col] == board->grid[row + 1][col])
                return BOARD_STATUS_PROCESS;
        }
    }

    return BOARD_STATUS_LOSE;
}

int board_get_data(const Board *board, int row, int col)
{
    if (board == NULL || row < 0 || row >= BOARD_ROWS ||
        col < 0 || col >= BOARD_COLS)
        return -1;

    return board->grid[row][col];
}

int board_get_score(const Board *board)
{
    return board == NULL ? 0 : board->score;
}

int board_get_step(const Board *board)
{
    return board == NULL ? 0 : board->step;
}

bool board_has_started(const Board *board)
{
    return board != NULL && board->game_start;
}

bool board_is_over(const Board *board)
{
    return board != NULL && board->game_over;
}
