#pragma once

#include <stdbool.h>
#include "config.h"

typedef struct HistoryEntry
{
    char username[USER_NAME_LENGTH_MAX];
    int score;
    int max_tile;
    int steps;
    int elapsed_seconds;
    char mode[GAME_MODE_ID_LENGTH];
    long long finished_at;
} HistoryEntry;

bool history_save(const char *history_file, const HistoryEntry *entry);
int history_load_user(const char *history_file,
                      const char *username,
                      HistoryEntry *entries,
                      int max_entries);
bool history_delete_user(const char *history_file, const char *username);
bool history_rename_user(const char *history_file,
                         const char *old_username,
                         const char *new_username);
