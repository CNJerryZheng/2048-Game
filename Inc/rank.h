#pragma once

#include <stdbool.h>

#include "config.h"

typedef struct
{
    char username[USER_NAME_LENGTH_MAX];
    int score;
} RankEntry;

void rank_show(const char *scores_file, const char *current_user);

bool rank_save_score(const char *scores_file, const char *username, int score);
