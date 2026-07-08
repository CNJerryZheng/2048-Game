#include "history.h"

#include "storage.h"
#include "utils.h"

#include <stdio.h>

static bool history_parse(const char *line, HistoryEntry *entry)
{
    char encrypted[512];
    char plain[256];

    if (sscanf(line, "%31[^\t]\t%511[^\r\n]",
               entry->username, encrypted) != 2 ||
        !utils_xor_decrypt_from_hex(encrypted, plain, sizeof(plain)))
        return false;
    return sscanf(plain, "%d|%d|%d|%d|%31[^|]|%lld",
                  &entry->score, &entry->max_tile, &entry->steps,
                  &entry->elapsed_seconds, entry->mode,
                  &entry->finished_at) == 6;
}

static bool history_write(FILE *file, const HistoryEntry *entry)
{
    char plain[256];
    char encrypted[512];

    if (snprintf(plain, sizeof(plain), "%d|%d|%d|%d|%s|%lld",
                 entry->score, entry->max_tile, entry->steps,
                 entry->elapsed_seconds, entry->mode,
                 entry->finished_at) < 0 ||
        !utils_xor_encrypt_to_hex(plain, encrypted, sizeof(encrypted)))
        return false;
    return fprintf(file, "%s\t%s\n", entry->username, encrypted) >= 0;
}

bool history_save(const char *history_file, const HistoryEntry *entry)
{
    FILE *file;
    bool success;

    if (history_file == NULL || entry == NULL || entry->username[0] == '\0')
        return false;
    file = storage_open_append(history_file);
    if (file == NULL)
        return false;
    success = history_write(file, entry);
    return storage_close(file) && success;
}

int history_load_user(const char *history_file,
                      const char *username,
                      HistoryEntry *entries,
                      int max_entries)
{
    FILE *file;
    HistoryEntry entry;
    char line[1024];
    int count = 0;

    if (history_file == NULL || username == NULL || entries == NULL)
        return 0;
    file = storage_open_read(history_file);
    if (file == NULL)
        return 0;
    while (storage_read_line(file, line, sizeof(line)))
    {
        if (history_parse(line, &entry) &&
            utils_string_equal(entry.username, username))
        {
            if (count < max_entries)
                entries[count++] = entry;
        }
    }
    (void)storage_close(file);
    return count;
}

bool history_delete_user(const char *history_file, const char *username)
{
    FILE *source;
    FILE *target;
    HistoryEntry entry;
    char line[1024];
    char cache_file[512];
    bool success = true;

    source = storage_open_read(history_file);
    if (source == NULL)
        return true;
    if (snprintf(cache_file, sizeof(cache_file), "%s.tmp", history_file) < 0)
    {
        (void)storage_close(source);
        return false;
    }
    target = storage_open_write(cache_file);
    if (target == NULL)
    {
        (void)storage_close(source);
        return false;
    }
    while (storage_read_line(source, line, sizeof(line)))
    {
        if (!history_parse(line, &entry))
            continue;
        if (!utils_string_equal(entry.username, username) &&
            !history_write(target, &entry))
            success = false;
    }
    success = storage_close(source) && storage_close(target) && success;
    if (!success || !storage_replace_file(cache_file, history_file))
    {
        (void)storage_remove_file(cache_file);
        return false;
    }
    return true;
}

bool history_rename_user(const char *history_file,
                         const char *old_username,
                         const char *new_username)
{
    FILE *source;
    FILE *target;
    HistoryEntry entry;
    char line[1024];
    char cache_file[512];
    bool success = true;
    source = storage_open_read(history_file);
    if (source == NULL)
        return true;
    if (snprintf(cache_file, sizeof(cache_file), "%s.tmp", history_file) < 0)
    {
        (void)storage_close(source);
        return false;
    }
    target = storage_open_write(cache_file);
    if (target == NULL)
    {
        (void)storage_close(source);
        return false;
    }
    while (storage_read_line(source, line, sizeof(line)))
    {
        if (!history_parse(line, &entry))
            continue;
        if (utils_string_equal(entry.username, old_username))
            utils_copy_string(entry.username, new_username, sizeof(entry.username));
        if (!history_write(target, &entry))
            success = false;
    }
    success = storage_close(source) && storage_close(target) && success;
    if (!success || !storage_replace_file(cache_file, history_file))
    {
        (void)storage_remove_file(cache_file);
        return false;
    }
    return true;
}
