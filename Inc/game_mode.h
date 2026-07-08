#pragma once

#include <stdbool.h>
#include "board.h"

typedef void (*GameModeStartHandler)(Board *board);
typedef bool (*GameModeProcessHandler)(Board *board, BoardCommand command);
typedef BoardStatus (*GameModeJudgeHandler)(const Board *board);

typedef struct GameModeDefinition
{
    char id[GAME_MODE_ID_LENGTH];
    char display_name[64];
    int target_tile;
    int step_limit;
    int time_limit_seconds;
    bool ranking_enabled;
    GameModeStartHandler start;
    GameModeProcessHandler process;
    GameModeJudgeHandler judge;
} GameModeDefinition;

bool game_mode_register(const GameModeDefinition *definition);
const GameModeDefinition *game_mode_find(const char *id);
int game_mode_count(void);
const GameModeDefinition *game_mode_at(int index);
bool game_mode_start(const char *id, Board *board);
bool game_mode_process(const char *id, Board *board, BoardCommand command);
BoardStatus game_mode_judge(const char *id, const Board *board);
