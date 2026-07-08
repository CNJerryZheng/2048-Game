#include "game_mode.h"

#include "board.h"
#include "utils.h"

#define GAME_MODE_MAX_COUNT 16

static GameModeDefinition modes[GAME_MODE_MAX_COUNT];
static int mode_count = 0;

static void game_mode_initialize(void)
{
    GameModeDefinition classic = {0};
    if (mode_count != 0)
        return;
    utils_copy_string(classic.id, "classic", sizeof(classic.id));
    utils_copy_string(classic.display_name, "经典模式", sizeof(classic.display_name));
    classic.target_tile = BOARD_TARGET;
    classic.step_limit = 0;
    classic.time_limit_seconds = 0;
    classic.ranking_enabled = true;
    classic.start = board_start;
    classic.process = board_process;
    classic.judge = board_judge;
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

bool game_mode_start(const char *id, Board *board)
{
    const GameModeDefinition *mode = game_mode_find(id);
    if (mode == NULL || board == NULL)
        return false;
    (mode->start == NULL ? board_start : mode->start)(board);
    utils_copy_string(board->mode, mode->id, sizeof(board->mode));
    return true;
}

bool game_mode_process(const char *id, Board *board, BoardCommand command)
{
    const GameModeDefinition *mode = game_mode_find(id);
    if (mode == NULL || board == NULL)
        return false;
    return (mode->process == NULL ? board_process : mode->process)(board, command);
}

BoardStatus game_mode_judge(const char *id, const Board *board)
{
    const GameModeDefinition *mode = game_mode_find(id);
    BoardStatus status;
    if (mode == NULL || board == NULL)
        return BOARD_STATUS_WAIT;
    status = (mode->judge == NULL ? board_judge : mode->judge)(board);
    if (status == BOARD_STATUS_PROCESS && mode->step_limit > 0 &&
        board->step >= mode->step_limit)
        return BOARD_STATUS_LOSE;
    if (status == BOARD_STATUS_PROCESS && mode->time_limit_seconds > 0 &&
        board->elapsed_seconds >= mode->time_limit_seconds)
        return BOARD_STATUS_LOSE;
    return status;
}
