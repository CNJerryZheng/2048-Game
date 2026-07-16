#include "board.h"
#include "config.h"
#include "utils.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>

static bool random_seed_initialized = false;

static void board_merge_line(int line[BOARD_MAX_SIZE], int length, int *score)
{
    int compacted[BOARD_MAX_SIZE] = {0};
    int write_index = 0;
    int read_index;

    for (read_index = 0; read_index < length; read_index = -~read_index)
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

    memcpy(line, compacted, (size_t)length * sizeof(compacted[0]));
}

static void board_move(Board *board, BoardCommand command)
{
    int line[BOARD_MAX_SIZE];
    int outer;
    int inner;
    const bool horizontal = command == BOARD_CMD_LEFT || command == BOARD_CMD_RIGHT;
    const int outer_count = horizontal ? board->rows : board->cols;
    const int line_length = horizontal ? board->cols : board->rows;

    for (outer = 0; outer < outer_count; outer = -~outer)
    {
        for (inner = 0; inner < line_length; inner = -~inner)
        {
            if (command == BOARD_CMD_LEFT)
                line[inner] = board->grid[outer][inner];
            else if (command == BOARD_CMD_RIGHT)
                line[inner] = board->grid[outer][board->cols - 1 - inner];
            else if (command == BOARD_CMD_UP)
                line[inner] = board->grid[inner][outer];
            else
                line[inner] = board->grid[board->rows - 1 - inner][outer];
        }

        board_merge_line(line, line_length, &board->score);

        for (inner = 0; inner < line_length; inner = -~inner)
        {
            if (command == BOARD_CMD_LEFT)
                board->grid[outer][inner] = line[inner];
            else if (command == BOARD_CMD_RIGHT)
                board->grid[outer][board->cols - 1 - inner] = line[inner];
            else if (command == BOARD_CMD_UP)
                board->grid[inner][outer] = line[inner];
            else
                board->grid[board->rows - 1 - inner][outer] = line[inner];
        }
    }
}

void board_init(Board *board)
{
    if (board != NULL)
    {
        memset(board, 0, sizeof(*board));
        board->rows = BOARD_DEFAULT_SIZE;
        board->cols = BOARD_DEFAULT_SIZE;
        board->target_tile = BOARD_TARGET;
    }
}

void board_set_size(Board *board, int rows, int cols)
{
    if (board == NULL)
        return;
    board->rows = rows < BOARD_MIN_SIZE ? BOARD_MIN_SIZE :
                  (rows > BOARD_MAX_SIZE ? BOARD_MAX_SIZE : rows);
    board->cols = cols < BOARD_MIN_SIZE ? BOARD_MIN_SIZE :
                  (cols > BOARD_MAX_SIZE ? BOARD_MAX_SIZE : cols);
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

    {
        const int rows = board->rows >= BOARD_MIN_SIZE && board->rows <= BOARD_MAX_SIZE
                             ? board->rows : BOARD_DEFAULT_SIZE;
        const int cols = board->cols >= BOARD_MIN_SIZE && board->cols <= BOARD_MAX_SIZE
                             ? board->cols : BOARD_DEFAULT_SIZE;
        board_init(board);
        board_set_size(board, rows, cols);
    }
    utils_copy_string(board->mode, "classic", sizeof(board->mode));
    (void)board_create_number(board);
    (void)board_create_number(board);
    board->game_start = true;
}

bool board_create_number(Board *board)
{
    int empty_rows[BOARD_MAX_SIZE * BOARD_MAX_SIZE];
    int empty_cols[BOARD_MAX_SIZE * BOARD_MAX_SIZE];
    int empty_count = 0;
    int row;
    int col;
    int selected;

    if (board == NULL)
        return false;

    for (row = 0; row < board->rows; row = -~row)
    {
        for (col = 0; col < board->cols; col = -~col)
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

    for (row = 0; row < board->rows; row = -~row)
    {
        for (col = 0; col < board->cols; col = -~col)
        {
            if (board->grid[row][col] >=
                (board->target_tile > 0 ? board->target_tile : BOARD_TARGET))
                return BOARD_STATUS_WIN;
        }
    }

    for (row = 0; row < board->rows; row = -~row)
    {
        for (col = 0; col < board->cols; col = -~col)
        {
            if (board->grid[row][col] == 0)
                return BOARD_STATUS_PROCESS;
        }
    }

    for (row = 0; row < board->rows; row = -~row)
    {
        for (col = 0; col < board->cols; col = -~col)
        {
            if (col + 1 < board->cols &&
                board->grid[row][col] == board->grid[row][col + 1])
                return BOARD_STATUS_PROCESS;
            if (row + 1 < board->rows &&
                board->grid[row][col] == board->grid[row + 1][col])
                return BOARD_STATUS_PROCESS;
        }
    }

    return BOARD_STATUS_LOSE;
}

int board_get_data(const Board *board, int row, int col)
{
    if (board == NULL || row < 0 || row >= board->rows ||
        col < 0 || col >= board->cols)
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

int board_get_max_tile(const Board *board)
{
    int row;
    int col;
    int maximum = 0;

    if (board == NULL)
        return 0;

    for (row = 0; row < board->rows; row = -~row)
    {
        for (col = 0; col < board->cols; col = -~col)
        {
            if (board->grid[row][col] > maximum)
                maximum = board->grid[row][col];
        }
    }
    return maximum;
}
