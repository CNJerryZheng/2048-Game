#include "storage.h"
#include "config.h"
#include "utils.h"

#include <stdlib.h>

#ifdef _WIN32
#include <windows.h>
#include <wchar.h>
#endif

#define SAVE_LINE_BUFFER_LENGTH 1024

typedef struct StorageSaveRecord
{
    char username[USER_NAME_LENGTH_MAX];
    Board board;
} StorageSaveRecord;

#ifdef _WIN32
static wchar_t *storage_utf8_to_wide(const char *text)
{
    int length;
    wchar_t *wide_text;

    if (text == NULL)
        return NULL;

    length = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                                 text, -1, NULL, 0);
    if (length <= 0)
        return NULL;

    wide_text = (wchar_t *)malloc((size_t)length * sizeof(wchar_t));
    if (wide_text == NULL)
        return NULL;

    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS,
                            text, -1, wide_text, length) <= 0)
    {
        free(wide_text);
        return NULL;
    }
    return wide_text;
}

static FILE *storage_open_utf8(const char *file_path, const wchar_t *mode)
{
    FILE *file;
    wchar_t *wide_path = storage_utf8_to_wide(file_path);
    if (wide_path == NULL)
        return NULL;
    file = _wfopen(wide_path, mode);
    free(wide_path);
    return file;
}
#endif

FILE *storage_open_read(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

#ifdef _WIN32
    return storage_open_utf8(file_path, L"r");
#else
    return fopen(file_path, "r");
#endif
}

FILE *storage_open_append(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

#ifdef _WIN32
    return storage_open_utf8(file_path, L"a");
#else
    return fopen(file_path, "a");
#endif
}

FILE *storage_open_write(const char *file_path)
{
    if (file_path == NULL)
        return NULL;

#ifdef _WIN32
    return storage_open_utf8(file_path, L"w");
#else
    return fopen(file_path, "w");
#endif
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

static bool storage_write_save_record(FILE *file,
                                      const char *username,
                                      const Board *board)
{
    char plain[SAVE_LINE_BUFFER_LENGTH / 2];
    char encrypted[SAVE_LINE_BUFFER_LENGTH];
    int length;
    int row;
    int col;

    if (file == NULL || username == NULL || username[0] == '\0' || board == NULL ||
        board->score < 0 || board->step < 0)
        return false;

    length = snprintf(plain, sizeof(plain), "v2|%d|%d|%d|%d|%d|%s|%d|%d",
                      board->score,
                      board->step,
                      board->game_start ? 1 : 0,
                      board->game_over ? 1 : 0,
                      board->elapsed_seconds,
                      board->mode[0] == '\0' ? "classic" : board->mode,
                      board->rows, board->cols);
    if (length < 0 || (size_t)length >= sizeof(plain))
        return false;

    for (row = 0; row < board->rows; row = -~row)
    {
        for (col = 0; col < board->cols; col = -~col)
        {
            int written = snprintf(plain + length, sizeof(plain) - (size_t)length,
                                   "|%d", board->grid[row][col]);
            if (written < 0 || (size_t)written >= sizeof(plain) - (size_t)length)
                return false;
            length += written;
        }
    }

    if (!utils_xor_encrypt_to_hex(plain, encrypted, sizeof(encrypted)))
        return false;
    return fprintf(file, "%s\t%s\n", username, encrypted) >= 0;
}

static char *storage_next_save_field(char **cursor)
{
    char *start;
    char *separator;
    if (cursor == NULL || *cursor == NULL || **cursor == '\0')
        return NULL;
    start = *cursor;
    separator = strchr(start, '|');
    if (separator == NULL)
        *cursor = start + strlen(start);
    else
    {
        *separator = '\0';
        *cursor = separator + 1;
    }
    return start;
}

static bool storage_parse_v2_save(char *plain,
                                  const char *parsed_username,
                                  char *username,
                                  size_t username_size,
                                  Board *board)
{
    char *cursor = plain;
    char *field;
    char mode[GAME_MODE_ID_LENGTH];
    int score, step, started, over, elapsed, rows, cols, row, col;

#define READ_SAVE_INT(target) do { field = storage_next_save_field(&cursor); if (field == NULL) return false; target = atoi(field); } while (0)
    field = storage_next_save_field(&cursor);
    if (field == NULL || strcmp(field, "v2") != 0)
        return false;
    READ_SAVE_INT(score);
    READ_SAVE_INT(step);
    READ_SAVE_INT(started);
    READ_SAVE_INT(over);
    READ_SAVE_INT(elapsed);
    field = storage_next_save_field(&cursor);
    if (field == NULL)
        return false;
    utils_copy_string(mode, field, sizeof(mode));
    READ_SAVE_INT(rows);
    READ_SAVE_INT(cols);
    if (score < 0 || step < 0 || (started != 0 && started != 1) ||
        (over != 0 && over != 1) || rows < BOARD_MIN_SIZE ||
        rows > BOARD_MAX_SIZE || cols < BOARD_MIN_SIZE || cols > BOARD_MAX_SIZE)
        return false;

    board_init(board);
    board_set_size(board, rows, cols);
    board->score = score;
    board->step = step;
    board->game_start = started != 0;
    board->game_over = over != 0;
    board->elapsed_seconds = elapsed < 0 ? 0 : elapsed;
    utils_copy_string(board->mode, mode[0] == '\0' ? "classic" : mode,
                      sizeof(board->mode));
    for (row = 0; row < rows; row = -~row)
    {
        for (col = 0; col < cols; col = -~col)
        {
            int value;
            READ_SAVE_INT(value);
            if (value < 0 || (value != 0 && (value & (value - 1)) != 0))
                return false;
            board->grid[row][col] = value;
        }
    }
    utils_copy_string(username, parsed_username, username_size);
#undef READ_SAVE_INT
    return true;
}

static bool storage_parse_save_line(const char *line,
                                    char *username,
                                    size_t username_size,
                                    Board *board)
{
    char parsed_username[USER_NAME_LENGTH_MAX];
    char encrypted[SAVE_LINE_BUFFER_LENGTH];
    char plain[SAVE_LINE_BUFFER_LENGTH / 2];
    char parsed_mode[GAME_MODE_ID_LENGTH];
    int values[4 + BOARD_ROWS * BOARD_COLS];
    int elapsed_seconds = 0;
    int matched;
    int index;

    if (line == NULL || username == NULL || username_size == 0 || board == NULL)
        return false;

    matched = sscanf(line, "%31[^\t]\t%1023[^\r\n]",
                     parsed_username, encrypted);
    if (matched == 2 &&
        utils_xor_decrypt_from_hex(encrypted, plain, sizeof(plain)))
    {
        if (strncmp(plain, "v2|", 3) == 0)
            return storage_parse_v2_save(plain, parsed_username,
                                         username, username_size, board);
        matched = sscanf(plain,
                         "%d|%d|%d|%d|%d|%31[^|]|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d|%d",
                         &values[0], &values[1], &values[2], &values[3],
                         &elapsed_seconds, parsed_mode,
                         &values[4], &values[5], &values[6], &values[7],
                         &values[8], &values[9], &values[10], &values[11],
                         &values[12], &values[13], &values[14], &values[15],
                         &values[16], &values[17], &values[18], &values[19]);
        if (matched == 6 + BOARD_DEFAULT_SIZE * BOARD_DEFAULT_SIZE)
            matched = 1 + 4 + BOARD_DEFAULT_SIZE * BOARD_DEFAULT_SIZE;
    }
    else
    {
        parsed_mode[0] = '\0';
        matched = sscanf(line,
                     "%31[^\t]\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d",
                     parsed_username,
                     &values[0], &values[1], &values[2], &values[3],
                     &values[4], &values[5], &values[6], &values[7],
                     &values[8], &values[9], &values[10], &values[11],
                     &values[12], &values[13], &values[14], &values[15],
                     &values[16], &values[17], &values[18], &values[19]);
    }

    if (matched != 1 + 4 + BOARD_DEFAULT_SIZE * BOARD_DEFAULT_SIZE ||
        values[0] < 0 || values[1] < 0 ||
        (values[2] != 0 && values[2] != 1) ||
        (values[3] != 0 && values[3] != 1))
        return false;

    for (index = 4; index < 4 + BOARD_DEFAULT_SIZE * BOARD_DEFAULT_SIZE; index = -~index)
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
    board->elapsed_seconds = elapsed_seconds < 0 ? 0 : elapsed_seconds;
    utils_copy_string(board->mode,
                      parsed_mode[0] == '\0' ? "classic" : parsed_mode,
                      sizeof(board->mode));

    for (index = 0; index < BOARD_DEFAULT_SIZE * BOARD_DEFAULT_SIZE; index = -~index)
        board->grid[index / BOARD_DEFAULT_SIZE][index % BOARD_DEFAULT_SIZE] = values[index + 4];

    return true;
}

static int storage_load_all_saves(const char *saves_file,
                                  StorageSaveRecord *entries,
                                  int max_entries)
{
    FILE *file;
    char line[SAVE_LINE_BUFFER_LENGTH];
    int count = 0;

    if (saves_file == NULL || entries == NULL || max_entries <= 0)
        return 0;

    file = storage_open_read(saves_file);
    if (file == NULL)
        return 0;

    while (count < max_entries && storage_read_line(file, line, sizeof(line)))
    {
        if (storage_parse_save_line(line,
                                    entries[count].username,
                                    sizeof(entries[count].username),
                                    &entries[count].board))
            count = -~count;
    }

    (void)storage_close(file);
    return count;
}

bool storage_save_exists(const char *saves_file, const char *username)
{
    return storage_load_save(saves_file, username, NULL);
}

bool storage_load_save(const char *saves_file,
                       const char *username,
                       Board *board)
{
    FILE *file;
    char line[SAVE_LINE_BUFFER_LENGTH];
    char parsed_username[USER_NAME_LENGTH_MAX];
    Board parsed_board;

    if (saves_file == NULL || username == NULL || username[0] == '\0')
        return false;

    file = storage_open_read(saves_file);
    if (file == NULL)
        return false;

    while (storage_read_line(file, line, sizeof(line)))
    {
        if (storage_parse_save_line(line,
                                    parsed_username,
                                    sizeof(parsed_username),
                                    &parsed_board) &&
            utils_string_equal(parsed_username, username))
        {
            if (board != NULL)
                *board = parsed_board;
            (void)storage_close(file);
            return true;
        }
    }

    (void)storage_close(file);
    return false;
}

bool storage_save_game(const char *saves_file,
                       const char *username,
                       const Board *board)
{
    StorageSaveRecord entries[SAVE_ENTRIES_MAX];
    char cache_file[INPUT_BUFFER_LENGTH];
    int cache_length;
    int count;
    int index;
    bool found = false;
    FILE *file;

    if (saves_file == NULL || username == NULL || username[0] == '\0' ||
        board == NULL)
        return false;

    cache_length = snprintf(cache_file, sizeof(cache_file), "%s.tmp", saves_file);
    if (cache_length < 0 || (size_t)cache_length >= sizeof(cache_file))
        return false;

    count = storage_load_all_saves(saves_file, entries, SAVE_ENTRIES_MAX);
    for (index = 0; index < count; index = -~index)
    {
        if (utils_string_equal(entries[index].username, username))
        {
            entries[index].board = *board;
            found = true;
            break;
        }
    }

    if (!found)
    {
        if (count >= SAVE_ENTRIES_MAX)
            return false;

        utils_copy_string(entries[count].username,
                          username,
                          sizeof(entries[count].username));
        entries[count].board = *board;
        count = -~count;
    }

    file = storage_open_write(cache_file);
    if (file == NULL)
        return false;

    for (index = 0; index < count; index = -~index)
    {
        if (!storage_write_save_record(file,
                                       entries[index].username,
                                       &entries[index].board))
        {
            (void)storage_close(file);
            (void)storage_remove_file(cache_file);
            return false;
        }
    }

    if (!storage_close(file) ||
        !storage_replace_file(cache_file, saves_file))
    {
        (void)storage_remove_file(cache_file);
        return false;
    }

    return true;
}

bool storage_delete_save(const char *saves_file, const char *username)
{
    StorageSaveRecord entries[SAVE_ENTRIES_MAX];
    char cache_file[INPUT_BUFFER_LENGTH];
    int cache_length;
    int count;
    int index;
    bool found = false;
    FILE *file;

    if (saves_file == NULL || username == NULL || username[0] == '\0')
        return false;

    cache_length = snprintf(cache_file, sizeof(cache_file), "%s.tmp", saves_file);
    if (cache_length < 0 || (size_t)cache_length >= sizeof(cache_file))
        return false;

    count = storage_load_all_saves(saves_file, entries, SAVE_ENTRIES_MAX);
    if (count == 0)
        return false;

    file = storage_open_write(cache_file);
    if (file == NULL)
        return false;

    for (index = 0; index < count; index = -~index)
    {
        if (utils_string_equal(entries[index].username, username))
        {
            found = true;
            continue;
        }

        if (!storage_write_save_record(file,
                                       entries[index].username,
                                       &entries[index].board))
        {
            (void)storage_close(file);
            (void)storage_remove_file(cache_file);
            return false;
        }
    }

    if (!storage_close(file))
    {
        (void)storage_remove_file(cache_file);
        return false;
    }

    if (!found)
    {
        (void)storage_remove_file(cache_file);
        return false;
    }

    if (!storage_replace_file(cache_file, saves_file))
    {
        (void)storage_remove_file(cache_file);
        return false;
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

#ifdef _WIN32
    {
        int result;
        wchar_t *wide_path = storage_utf8_to_wide(file_path);
        if (wide_path == NULL)
            return false;
        result = _wremove(wide_path);
        free(wide_path);
        return result == 0;
    }
#else
    return remove(file_path) == 0;
#endif
}

bool storage_replace_file(const char *source_path, const char *target_path)
{
    if (source_path == NULL || target_path == NULL)
        return false;

#ifdef _WIN32
    {
        bool success;
        wchar_t *wide_source = storage_utf8_to_wide(source_path);
        wchar_t *wide_target = storage_utf8_to_wide(target_path);
        if (wide_source == NULL || wide_target == NULL)
        {
            free(wide_source);
            free(wide_target);
            return false;
        }
        success = MoveFileExW(wide_source, wide_target,
                              MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
        free(wide_source);
        free(wide_target);
        return success;
    }
#else
    return rename(source_path, target_path) == 0;
#endif
}
