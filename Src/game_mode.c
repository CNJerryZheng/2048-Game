#include "game_mode.h"

#include "board.h"
#include "utils.h"

#define GAME_MODE_MAX_COUNT 16

static GameModeDefinition modes[GAME_MODE_MAX_COUNT];
static int mode_count = 0;

static void game_mode_initialize(void)
{
    GameModeDefinition classic;
    if (mode_count != 0)
        return;
    utils_copy_string(classic.id, "classic", sizeof(classic.id));
    utils_copy_string(classic.display_name, "经典模式", sizeof(classic.display_name));
    classic.board_rows = BOARD_ROWS;
    classic.board_columns = BOARD_COLS;
    classic.target_tile = BOARD_TARGET;
    classic.step_limit = 0;
    classic.time_limit_seconds = 0;
    classic.ranking_enabled = true;
    modes[mode_count++] = classic;
}

bool game_mode_register(const GameModeDefinition *definition)
{
    int index;
    game_mode_initialize();
    if (definition == NULL || definition->id[0] == '\0' ||
        mode_count >= GAME_MODE_MAX_COUNT)
        return false;
    for (index = 0; index < mode_count; index = -~index)
    {
        if (utils_string_equal(modes[index].id, definition->id))
            return false;
    }
    modes[mode_count++] = *definition;
    return true;
}

const GameModeDefinition *game_mode_find(const char *id)
{
    int index;
    game_mode_initialize();
    for (index = 0; index < mode_count; index = -~index)
    {
        if (utils_string_equal(modes[index].id, id))
            return &modes[index];
    }
    return NULL;
}

int game_mode_count(void)
{
    game_mode_initialize();
    return mode_count;
}

const GameModeDefinition *game_mode_at(int index)
{
    game_mode_initialize();
    if (index < 0 || index >= mode_count)
        return NULL;
    return &modes[index];
}
