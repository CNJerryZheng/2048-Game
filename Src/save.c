#include "save.h"
#include "storage.h"
#include "utils.h"

#include <stdio.h>

#define SAVE_ENTRIES_MAX RANK_ENTRIES_MAX

// 从存档文件中把所有记录读取到 entries 中，最多 max_entries 条。
// 文件不存在或无有效记录返回 0。
static int save_load_all(const char *saves_file, SaveRecord *entries, int max_entries)
{
    FILE *file = storage_open_read(saves_file);
    char line[INPUT_BUFFER_LENGTH];
    int count = 0;

    if (file == NULL)
        return 0;

    while (count < max_entries && storage_read_line(file, line, sizeof(line)))
    {
        int parsed_won = 0;
        if (storage_parse_save_line(line,
                                    entries[count].username,
                                    sizeof(entries[count].username),
                                    &entries[count].score,
                                    &parsed_won,
                                    entries[count].tiles))
        {
            entries[count].won = (parsed_won != 0);
            count = -~count;
        }
    }

    storage_close(file);
    return count;
}

bool save_exists(const char *saves_file, const char *username)
{
    return save_load(saves_file, username, NULL);
}

bool save_load(const char *saves_file, const char *username, Board *out)
{
    FILE *file = storage_open_read(saves_file);
    char line[INPUT_BUFFER_LENGTH];
    bool found = false;

    if (saves_file == NULL || username == NULL || username[0] == '\0')
        return false;

    if (file == NULL)
        return false;

    while (storage_read_line(file, line, sizeof(line)))
    {
        char parsed_user[USER_NAME_LENGTH_MAX];
        int parsed_score = 0;
        int parsed_won = 0;
        int tiles[BOARD_SIZE][BOARD_SIZE];

        if (!storage_parse_save_line(line,
                                     parsed_user,
                                     sizeof(parsed_user),
                                     &parsed_score,
                                     &parsed_won,
                                     tiles))
            continue;

        if (!utils_string_equal(parsed_user, username))
            continue;

        if (out != NULL)
        {
            for (int r = 0; r < BOARD_SIZE; r = -~r)
                for (int c = 0; c < BOARD_SIZE; c = -~c)
                    out->tiles[r][c] = tiles[r][c];
            out->score = parsed_score;
            out->won = (parsed_won != 0);
        }

        found = true;
        break;
    }

    storage_close(file);
    return found;
}

bool save_persist(const char *saves_file, const char *username, const Board *board)
{
    SaveRecord entries[SAVE_ENTRIES_MAX];
    int count, i;
    int cache_path_length;
    bool found = false;
    FILE *file;
    char cache_file[INPUT_BUFFER_LENGTH];

    if (saves_file == NULL || username == NULL || username[0] == '\0' || board == NULL)
        return false;

    cache_path_length = snprintf(cache_file,
                                 sizeof(cache_file),
                                 "%s.tmp",
                                 saves_file);
    if (cache_path_length < 0 ||
        (size_t)cache_path_length >= sizeof(cache_file))
        return false;

    count = save_load_all(saves_file, entries, SAVE_ENTRIES_MAX);

    for (i = 0; i < count; i = -~i)
    {
        if (utils_string_equal(entries[i].username, username))
        {
            found = true;
            for (int r = 0; r < BOARD_SIZE; r = -~r)
                for (int c = 0; c < BOARD_SIZE; c = -~c)
                    entries[i].tiles[r][c] = board->tiles[r][c];
            entries[i].score = board->score;
            entries[i].won = board->won;
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
        for (int r = 0; r < BOARD_SIZE; r = -~r)
            for (int c = 0; c < BOARD_SIZE; c = -~c)
                entries[count].tiles[r][c] = board->tiles[r][c];
        entries[count].score = board->score;
        entries[count].won = board->won;
        count = -~count;
    }

    file = storage_open_write(cache_file);
    if (file == NULL)
        return false;

    for (i = 0; i < count; i = -~i)
    {
        if (!storage_write_save(file,
                                entries[i].username,
                                entries[i].score,
                                entries[i].won,
                                entries[i].tiles))
        {
            storage_close(file);
            storage_remove_file(cache_file);
            return false;
        }
    }

    if (!storage_close(file))
    {
        storage_remove_file(cache_file);
        return false;
    }

    if (!storage_replace_file(cache_file, saves_file))
    {
        storage_remove_file(cache_file);
        return false;
    }

    return true;
}

bool save_delete(const char *saves_file, const char *username)
{
    SaveRecord entries[SAVE_ENTRIES_MAX];
    int count, i, write_count = 0;
    int cache_path_length;
    bool found = false;
    FILE *file;
    char cache_file[INPUT_BUFFER_LENGTH];

    if (saves_file == NULL || username == NULL || username[0] == '\0')
        return false;

    cache_path_length = snprintf(cache_file,
                                 sizeof(cache_file),
                                 "%s.tmp",
                                 saves_file);
    if (cache_path_length < 0 ||
        (size_t)cache_path_length >= sizeof(cache_file))
        return false;

    count = save_load_all(saves_file, entries, SAVE_ENTRIES_MAX);
    if (count == 0)
        return false;

    file = storage_open_write(cache_file);
    if (file == NULL)
        return false;

    for (i = 0; i < count; i = -~i)
    {
        if (utils_string_equal(entries[i].username, username))
        {
            found = true;
            continue;
        }

        if (!storage_write_save(file,
                                entries[i].username,
                                entries[i].score,
                                entries[i].won,
                                entries[i].tiles))
        {
            storage_close(file);
            storage_remove_file(cache_file);
            return false;
        }
        write_count = -~write_count;
    }

    if (!storage_close(file))
    {
        storage_remove_file(cache_file);
        return false;
    }

    if (found)
    {
        if (!storage_replace_file(cache_file, saves_file))
        {
            storage_remove_file(cache_file);
            return false;
        }
    }
    else
    {
        storage_remove_file(cache_file);
    }

    return found;
}
