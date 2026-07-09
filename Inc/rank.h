#pragma once

#include <stdbool.h>

#include "config.h"

typedef struct RankEntry
{
    char username[USER_NAME_LENGTH_MAX];
    int score;
    int max_tile;
    int steps;
    int elapsed_seconds;
    char mode[GAME_MODE_ID_LENGTH];
    bool deleted;
    long long achieved_at;
} RankEntry;

int rank_load_scores(const char *scores_file,
                     RankEntry *entries,
                     int max_entries);
bool rank_save_score(const char *scores_file,
                     const char *username,
                     int score,
                     int max_tile,
                     int steps,
                     int elapsed_seconds,
                     const char *mode);
bool rank_delete_user(const char *scores_file, const char *username);
bool rank_rename_user(const char *scores_file,
                      const char *old_username,
                      const char *new_username);
bool rank_mark_deleted(const char *scores_file, const char *username);
