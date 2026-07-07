#include "save.h"

#include "config.h"
#include "storage.h"
#include "utils.h"

#include <stdio.h>

#define SAVE_LINE_BUFFER_LENGTH 512

typedef struct SaveRecord
{
    char username[USER_NAME_LENGTH_MAX];
    Board board;
} SaveRecord;

static int save_load_all(const char *saves_file,
                         SaveRecord *entries,
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

bool save_exists(const char *saves_file, const char *username)
{
    return save_load(saves_file, username, NULL);
}

bool save_load(const char *saves_file, const char *username, Board *board)
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

bool save_persist(const char *saves_file,
                  const char *username,
                  const Board *board)
{
    SaveRecord entries[SAVE_ENTRIES_MAX];
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

    count = save_load_all(saves_file, entries, SAVE_ENTRIES_MAX);
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
        if (!storage_write_save(file,
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

bool save_delete(const char *saves_file, const char *username)
{
    SaveRecord entries[SAVE_ENTRIES_MAX];
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

    count = save_load_all(saves_file, entries, SAVE_ENTRIES_MAX);
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

        if (!storage_write_save(file,
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
