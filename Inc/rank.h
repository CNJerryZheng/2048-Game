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
