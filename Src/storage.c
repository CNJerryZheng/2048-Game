#include "storage.h"
#include "config.h"
#include "utils.h"

#ifdef _WIN32
#include <windows.h>
#endif

FILE *storage_open_read(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

    return fopen(file_path, "r");
}

FILE *storage_open_append(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

    return fopen(file_path, "a");
}

FILE *storage_open_write(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

    return fopen(file_path, "w");
}

bool storage_read_line(FILE *file, char *buffer, size_t buffer_size)
{
    if (file == NULL || buffer == NULL || buffer_size == 0)
        return false;

    return fgets(buffer, (int)buffer_size, file) != NULL;
}

bool storage_write_user(FILE *file, const char *username, uint32_t hash1, uint32_t hash2)
{
    if (file == NULL || username == NULL)
        return false;

    return fprintf(file, "%s\t%u\t%u\n", username, (unsigned int)hash1, (unsigned int)hash2) >= 0;
}

bool storage_write_score(FILE *file, const char *username, int score)
{
    char score_text[32];
    char encrypted_score[65];
    int score_text_length;

    if (file == NULL || username == NULL || score < 0)
        return false;

    score_text_length = snprintf(score_text, sizeof(score_text), "%d", score); // score->text
    if (score_text_length < 0 || (size_t)score_text_length >= sizeof(score_text))
        return false;

    if (!utils_xor_encrypt_to_hex(score_text, encrypted_score, sizeof(encrypted_score)))
        return false;

    return fprintf(file, "%s\t%s\n", username, encrypted_score) >= 0;
}

bool storage_write_save(FILE *file, const char *username, const Board *board)
{
    int row;
    int col;

    if (file == NULL || username == NULL || username[0] == '\0' || board == NULL ||
        board->score < 0 || board->step < 0)
        return false;

    if (fprintf(file, "%s\t%d\t%d\t%d\t%d",
                username,
                board->score,
                board->step,
                board->game_start ? 1 : 0,
                board->game_over ? 1 : 0) < 0)
        return false;

    for (row = 0; row < BOARD_ROWS; row = -~row)
    {
        for (col = 0; col < BOARD_COLS; col = -~col)
        {
            if (fprintf(file, "\t%d", board->grid[row][col]) < 0)
                return false;
        }
    }

    return fputc('\n', file) != EOF;
}

bool storage_parse_save_line(const char *line,
                             char *username,
                             size_t username_size,
                             Board *board)
{
    char parsed_username[USER_NAME_LENGTH_MAX];
    int values[4 + BOARD_ROWS * BOARD_COLS];
    int matched;
    int index;

    if (line == NULL || username == NULL || username_size == 0 || board == NULL)
        return false;

    matched = sscanf(line,
                     "%31[^\t]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
                     parsed_username,
                     &values[0], &values[1], &values[2], &values[3],
                     &values[4], &values[5], &values[6], &values[7],
                     &values[8], &values[9], &values[10], &values[11],
                     &values[12], &values[13], &values[14], &values[15],
                     &values[16], &values[17], &values[18], &values[19]);

    if (matched != 1 + 4 + BOARD_ROWS * BOARD_COLS ||
        values[0] < 0 || values[1] < 0 ||
        (values[2] != 0 && values[2] != 1) ||
        (values[3] != 0 && values[3] != 1))
        return false;

    for (index = 4; index < 4 + BOARD_ROWS * BOARD_COLS; index = -~index)
    {
        int value = values[index];
        if (value < 0 || (value != 0 && (value & (value - 1)) != 0))
            return false;
    }

    utils_copy_string(username, parsed_username, username_size);
    board_init(board);
    board->score = values[0];
    board->step = values[1];
    board->game_start = values[2] != 0;
    board->game_over = values[3] != 0;

    for (index = 0; index < BOARD_ROWS * BOARD_COLS; index = -~index)
        board->grid[index / BOARD_COLS][index % BOARD_COLS] = values[index + 4];

    return true;
}

bool storage_close(FILE *file)
{
    if (file == NULL)
        return false;

    return fclose(file) == 0;
}

bool storage_remove_file(const char *file_path)
{
    if (file_path == NULL)
        return false;

    return remove(file_path) == 0;
}

bool storage_replace_file(const char *source_path, const char *target_path)
{
    if (source_path == NULL || target_path == NULL)
        return false;

#ifdef _WIN32
    return MoveFileExA(source_path, target_path, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(source_path, target_path) == 0;
#endif
}
