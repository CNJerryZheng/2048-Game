#include "storage.h"
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

bool storage_write_user(FILE *file, const char *username, const char *password)
{
    if (file == NULL || username == NULL || password == NULL)
        return false;

    return fprintf(file, "%s\t%s\n", username, password) >= 0;
}

bool storage_write_score(FILE *file, const char *username, int score)
{
    if (file == NULL || username == NULL || score < 0)
        return false;

    return fprintf(file, "%s\t%d\n", username, score) >= 0;
}

bool storage_write_save(FILE *file,
                        const char *username,
                        int score,
                        int won,
                        const int tiles[BOARD_SIZE][BOARD_SIZE])
{
    if (file == NULL || username == NULL || tiles == NULL)
        return false;

    return fprintf(file,
                   "%s\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                   username,
                   score,
                   won ? 1 : 0,
                   tiles[0][0], tiles[0][1], tiles[0][2], tiles[0][3],
                   tiles[1][0], tiles[1][1], tiles[1][2], tiles[1][3],
                   tiles[2][0], tiles[2][1], tiles[2][2], tiles[2][3],
                   tiles[3][0], tiles[3][1], tiles[3][2], tiles[3][3]) >= 0;
}

bool storage_parse_save_line(const char *line,
                             char *username,
                             size_t username_size,
                             int *score,
                             int *won,
                             int tiles[BOARD_SIZE][BOARD_SIZE])
{
    char parsed_user[USER_NAME_LENGTH_MAX];
    int parsed_score = 0;
    int parsed_won = 0;
    int t[BOARD_SIZE * BOARD_SIZE];
    int matched;

    if (line == NULL || username == NULL || username_size == 0 ||
        score == NULL || won == NULL || tiles == NULL)
        return false;

    matched = sscanf(line,
                     "%31[^\t]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
                     parsed_user,
                     &parsed_score,
                     &parsed_won,
                     &t[0],  &t[1],  &t[2],  &t[3],
                     &t[4],  &t[5],  &t[6],  &t[7],
                     &t[8],  &t[9],  &t[10], &t[11],
                     &t[12], &t[13], &t[14], &t[15]);

    if (matched != 1 + 1 + 1 + BOARD_SIZE * BOARD_SIZE)
        return false;

    if (parsed_score < 0)
        return false;

    utils_copy_string(username, parsed_user, username_size);
    *score = parsed_score;
    *won = (parsed_won != 0);

    for (int i = 0; i < BOARD_SIZE * BOARD_SIZE; i = -~i)
    {
        int row = i / BOARD_SIZE;
        int col = i % BOARD_SIZE;
        tiles[row][col] = t[i];
    }

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
    return MoveFileExA(source_path,
                       target_path,
                       MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(source_path, target_path) == 0;
#endif
}
