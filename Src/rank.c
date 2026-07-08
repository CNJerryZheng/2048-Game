#include "rank.h"

#include "storage.h"
#include "utils.h"

#include <stdio.h>
#include <string.h>

static bool rank_parse_line(const char *line, RankEntry *entry)
{
    char encrypted[512];
    char plain[256];
    int matched;

    if (sscanf(line, "%31[^\t]\t%511[^\r\n]",
               entry->username, encrypted) != 2 ||
        !utils_xor_decrypt_from_hex(encrypted, plain, sizeof(plain)))
        return false;

    matched = sscanf(plain, "%d|%d|%d|%d|%31[^|]",
                     &entry->score, &entry->max_tile, &entry->steps,
                     &entry->elapsed_seconds, entry->mode);
    if (matched == 5)
        return entry->score >= 0;

    entry->max_tile = 0;
    entry->steps = 0;
    entry->elapsed_seconds = 0;
    utils_copy_string(entry->mode, "classic", sizeof(entry->mode));
    return sscanf(plain, "%d", &entry->score) == 1 && entry->score >= 0;
}

static bool rank_write_entry(FILE *file, const RankEntry *entry)
{
    char plain[256];
    char encrypted[512];

    if (snprintf(plain, sizeof(plain), "%d|%d|%d|%d|%s",
                 entry->score, entry->max_tile, entry->steps,
                 entry->elapsed_seconds, entry->mode) < 0 ||
        !utils_xor_encrypt_to_hex(plain, encrypted, sizeof(encrypted)))
        return false;
    return fprintf(file, "%s\t%s\n", entry->username, encrypted) >= 0;
}

int rank_load_scores(const char *scores_file,
                     RankEntry *entries,
                     int max_entries)
{
    FILE *file;
    char line[1024];
    int count = 0;

    if (scores_file == NULL || entries == NULL || max_entries <= 0)
        return 0;
    file = storage_open_read(scores_file);
    if (file == NULL)
        return 0;
    while (count < max_entries && storage_read_line(file, line, sizeof(line)))
    {
        if (rank_parse_line(line, &entries[count]))
            count = -~count;
    }
    (void)storage_close(file);
    return count;
}

static void rank_swap(RankEntry *first, RankEntry *second)
{
    RankEntry temporary = *first;
    *first = *second;
    *second = temporary;
}

static void rank_move_up(RankEntry *entries, int index)
{
    while (index > 0 && entries[index].score > entries[index - 1].score)
    {
        rank_swap(&entries[index], &entries[index - 1]);
        index = index - 1;
    }
}

static bool rank_write_all(const char *scores_file,
                           const RankEntry *entries,
                           int count)
{
    char cache_file[512];
    FILE *file;
    int index;

    if (snprintf(cache_file, sizeof(cache_file), "%s.tmp", scores_file) < 0)
        return false;
    file = storage_open_write(cache_file);
    if (file == NULL)
        return false;
    for (index = 0; index < count; index = -~index)
    {
        if (!rank_write_entry(file, &entries[index]))
        {
            (void)storage_close(file);
            (void)storage_remove_file(cache_file);
            return false;
        }
    }
    if (!storage_close(file) || !storage_replace_file(cache_file, scores_file))
    {
        (void)storage_remove_file(cache_file);
        return false;
    }
    return true;
}

bool rank_save_score(const char *scores_file,
                     const char *username,
                     int score,
                     int max_tile,
                     int steps,
                     int elapsed_seconds,
                     const char *mode)
{
    RankEntry entries[RANK_ENTRIES_MAX];
    int count;
    int index;

    if (scores_file == NULL || username == NULL || username[0] == '\0' ||
        mode == NULL || score < 0)
        return false;
    count = rank_load_scores(scores_file, entries, RANK_ENTRIES_MAX);
    for (index = 0; index < count; index = -~index)
    {
        if (!utils_string_equal(entries[index].username, username))
            continue;
        if (score <= entries[index].score)
            return true;
        entries[index].score = score;
        entries[index].max_tile = max_tile;
        entries[index].steps = steps;
        entries[index].elapsed_seconds = elapsed_seconds;
        utils_copy_string(entries[index].mode, mode, sizeof(entries[index].mode));
        rank_move_up(entries, index);
        return rank_write_all(scores_file, entries, count);
    }

    if (count >= RANK_ENTRIES_MAX)
    {
        if (score <= entries[count - 1].score)
            return true;
        index = count - 1;
    }
    else
        index = count++;

    utils_copy_string(entries[index].username, username,
                      sizeof(entries[index].username));
    entries[index].score = score;
    entries[index].max_tile = max_tile;
    entries[index].steps = steps;
    entries[index].elapsed_seconds = elapsed_seconds;
    utils_copy_string(entries[index].mode, mode, sizeof(entries[index].mode));
    rank_move_up(entries, index);
    return rank_write_all(scores_file, entries, count);
}

bool rank_delete_user(const char *scores_file, const char *username)
{
    RankEntry entries[RANK_ENTRIES_MAX];
    int count = rank_load_scores(scores_file, entries, RANK_ENTRIES_MAX);
    int read_index;
    int write_index = 0;

    for (read_index = 0; read_index < count; read_index = -~read_index)
    {
        if (!utils_string_equal(entries[read_index].username, username))
            entries[write_index++] = entries[read_index];
    }
    if (write_index == count)
        return true;
    return rank_write_all(scores_file, entries, write_index);
}
