#pragma once

#include <stdbool.h>
#include "config.h"

typedef struct GameModeDefinition
{
    char id[GAME_MODE_ID_LENGTH];
    char display_name[64];
    int board_rows;
    int board_columns;
    int target_tile;
    int step_limit;
    int time_limit_seconds;
    bool ranking_enabled;
} GameModeDefinition;

bool game_mode_register(const GameModeDefinition *definition);
const GameModeDefinition *game_mode_find(const char *id);
int game_mode_count(void);
const GameModeDefinition *game_mode_at(int index);
