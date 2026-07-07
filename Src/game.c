#include "game.h"

#include "board.h"
#include "config.h"
#include "rank.h"
#include "save.h"
#include "ui.h"

#include <stdbool.h>
#include <stdio.h>

#ifdef _WIN32
#include <conio.h>
#endif

typedef enum GameInput
{
    GAME_INPUT_INVALID,
    GAME_INPUT_MOVE,
    GAME_INPUT_SAVE,
    GAME_INPUT_QUIT
} GameInput;

static int game_read_key(void)
{
#ifdef _WIN32
    return _getch();
#else
    return getchar();
#endif
}

static GameInput game_read_input(BoardCommand *command)
{
    int key = game_read_key();

#ifdef _WIN32
    if (key == 0 || key == 224)
    {
        key = game_read_key();
        if (key == 72)
            *command = BOARD_CMD_UP;
        else if (key == 80)
            *command = BOARD_CMD_DOWN;
        else if (key == 75)
            *command = BOARD_CMD_LEFT;
        else if (key == 77)
            *command = BOARD_CMD_RIGHT;
        else
            return GAME_INPUT_INVALID;
        return GAME_INPUT_MOVE;
    }
#endif

    if (key == 'w' || key == 'W')
        *command = BOARD_CMD_UP;
    else if (key == 's' || key == 'S')
        *command = BOARD_CMD_DOWN;
    else if (key == 'a' || key == 'A')
        *command = BOARD_CMD_LEFT;
    else if (key == 'd' || key == 'D')
        *command = BOARD_CMD_RIGHT;
    else if (key == 'p' || key == 'P')
        return GAME_INPUT_SAVE;
    else if (key == 'q' || key == 'Q')
        return GAME_INPUT_QUIT;
    else
        return GAME_INPUT_INVALID;

    return GAME_INPUT_MOVE;
}

void game_run(const char *current_user)
{
    Board board;
    BoardStatus status = BOARD_STATUS_PROCESS;
    bool loaded = false;
    bool quit = false;
    const char *message = NULL;

    if (current_user == NULL || current_user[0] == '\0')
        return;

    if (save_load(SAVES_DATA_FILE, current_user, &board))
    {
        if (ui_ask_continue_save(board.score))
        {
            board.game_over = false;
            loaded = true;
        }
        else
        {
            (void)save_delete(SAVES_DATA_FILE, current_user);
        }
    }

    if (!loaded)
        board_start(&board);

    while (!quit)
    {
        BoardCommand command = BOARD_CMD_UP;
        GameInput input;

        ui_show_game_state(&board, current_user);
        ui_show_game_message(message);
        message = NULL;
        input = game_read_input(&command);

        if (input == GAME_INPUT_QUIT)
        {
            quit = true;
        }
        else if (input == GAME_INPUT_SAVE)
        {
            message = save_persist(SAVES_DATA_FILE, current_user, &board)
                          ? "存档成功。"
                          : "存档失败，请检查存档文件。";
        }
        else if (input == GAME_INPUT_MOVE)
        {
            if (!board_process(&board, command))
            {
                message = "这个方向无法移动。";
                continue;
            }

            status = board_judge(&board);
            if (status == BOARD_STATUS_WIN || status == BOARD_STATUS_LOSE)
                break;
        }
        else
        {
            message = "无效按键，请使用方向键或 W/A/S/D。";
        }
    }

    if (status == BOARD_STATUS_WIN || status == BOARD_STATUS_LOSE)
    {
        bool rank_saved = rank_save_score(SCORES_DATA_FILE,
                                          current_user,
                                          board.score);
        (void)save_delete(SAVES_DATA_FILE, current_user);
        ui_show_game_over(&board, status, rank_saved);
    }
    else
    {
        bool saved = save_persist(SAVES_DATA_FILE, current_user, &board);
        ui_show_quit_save_result(saved);
    }

    ui_wait_for_enter();
}
