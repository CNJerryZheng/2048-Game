#include "game.h"
#include "config.h"
#include "board.h"
#include "ui.h"
#include "rank.h"
#include "save.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

#ifdef _WIN32
#include <conio.h>
#endif

/* ---------- 内部工具 ---------- */

static void game_show_header(const Board *board, const char *current_user)
{
    int max_tile = 0;

    if (current_user != NULL)
        printf("玩家：%s\n", current_user);

    printf("当前分数：%d\n", board->score);
    board_get_max_tile(board, &max_tile);
    printf("最大方块：%d\n", max_tile);
    puts("--------------------------------");
}

static void game_show_help(void)
{
    puts("");
    puts("操作说明：");
    puts("  W / 上箭头   方块向上合并");
    puts("  S / 下箭头   方块向下合并");
    puts("  A / 左箭头   方块向左合并");
    puts("  D / 右箭头   方块向右合并");
    puts("  P            保存当前进度并继续");
    puts("  Q            放弃本局并返回主菜单");
    puts("--------------------------------");
}

static void game_show_screen(const Board *board, const char *current_user)
{
    ui_clear_screen();
    puts("================================");
    puts("            2048 游戏            ");
    puts("================================");
    game_show_header(board, current_user);
    board_render(board);
    game_show_help();
}

static void game_wait_for_key(void)
{
    int ch;

    puts("");
    puts("按任意键继续...");
#ifdef _WIN32
    (void)_getch();
#else
    (void)getchar();
    /* 清掉行尾残留字符 */
    while ((ch = getchar()) != '\n' && ch != EOF)
    {
    }
#endif
}

// 把 getch / getchar 获得的字符翻译为方向。
// 返回 true 表示解析成功，*direction 写入方向；返回 false 表示玩家想要退出。
static bool game_parse_key(int key, BoardDirection *direction, bool *quit)
{
    *quit = false;
    *direction = BOARD_DIR_UP;

    /* 字母（大小写不敏感） */
    if (key == 'w' || key == 'W')
    {
        *direction = BOARD_DIR_UP;
        return true;
    }
    if (key == 's' || key == 'S')
    {
        *direction = BOARD_DIR_DOWN;
        return true;
    }
    if (key == 'a' || key == 'A')
    {
        *direction = BOARD_DIR_LEFT;
        return true;
    }
    if (key == 'd' || key == 'D')
    {
        *direction = BOARD_DIR_RIGHT;
        return true;
    }
    if (key == 'q' || key == 'Q')
    {
        *quit = true;
        return false;
    }

    /* 转义序列：方向键 */
    if (key == 27) // ESC
    {
        int next1 = 0;
        int next2 = 0;
#ifdef _WIN32
        if (_kbhit())
            next1 = _getch();
        if (next1 == '[' && _kbhit())
            next2 = _getch();
#else
        next1 = getchar();
        if (next1 == '[')
            next2 = getchar();
#endif
        if (next1 == '[')
        {
            switch (next2)
            {
            case 'A': *direction = BOARD_DIR_UP;    return true; // 上箭头
            case 'B': *direction = BOARD_DIR_DOWN;  return true; // 下箭头
            case 'C': *direction = BOARD_DIR_RIGHT; return true; // 右箭头
            case 'D': *direction = BOARD_DIR_LEFT;  return true; // 左箭头
            }
        }
    }

    return false;
}

// 读取一次按键（无需回车）。
static int game_read_key(void)
{
#ifdef _WIN32
    return _getch();
#else
    int ch = getchar();
    if (ch == '\r')
        ch = '\n';
    return ch;
#endif
}

// 展示游戏结束界面
static void game_show_over(const Board *board, bool reached_2048)
{
    ui_clear_screen();
    puts("================================");
    puts("            游戏结束            ");
    puts("================================");
    game_show_header(board, "");
    board_render(board);
    puts("");
    if (reached_2048)
        puts("恭喜你合成 2048！可以选择继续挑战或返回主菜单。");
    else
        puts("很遗憾，没有可合并的方块了。");
    puts("");
    puts("本局得分已自动保存到排行榜。");
    game_wait_for_key();
}

/* ---------- 对外接口 ---------- */

void game_run(const char *current_user)
{
    Board board;
    bool reached_2048 = false;
    bool loaded_from_save = false;

    if (current_user == NULL || current_user[0] == '\0')
    {
        puts("请先登录后再开始游戏！");
        ui_wait_for_enter();
        return;
    }

    /* 1) 先看是否有存档：有则询问玩家"继续"或"重新开始" */
    if (save_exists(SAVES_DATA_FILE, current_user))
    {
        Board preview;
        if (save_load(SAVES_DATA_FILE, current_user, &preview))
        {
            bool continue_game = ui_ask_continue_save(preview.score, preview.won);
            if (continue_game)
            {
                board = preview;
                reached_2048 = board.won;
                loaded_from_save = true;
            }
            else
            {
                /* 玩家选择重新开始，清除旧存档 */
                (void)save_delete(SAVES_DATA_FILE, current_user);
            }
        }
    }

    /* 2) 如果没有从存档恢复，则开新局 */
    if (!loaded_from_save)
    {
        board_initialize(&board);
    }

    /* 3) 仅在游戏开始时渲染一次棋盘；之后只在成功移动后重渲染 */
    game_show_screen(&board, current_user);

    while (true)
    {
        printf("请按方向键或 W/A/S/D 操作（Q 退出 / P 存档）：");

        int key = game_read_key();

        /* 在类 Unix 系统上，输入回车会产生 '\n'，视作无效输入并继续等待 */
        if (key == '\n' || key == '\r')
        {
            continue;
        }

        BoardDirection direction;
        bool quit = false;
        if (!game_parse_key(key, &direction, &quit))
        {
            if (quit)
                break;

            /* P 键：保存当前进度到 Data/saves.txt */
            if (key == 'p' || key == 'P')
            {
                if (save_persist(SAVES_DATA_FILE, current_user, &board))
                    ui_show_save_success(board.score);
                else
                    puts("  ← 存档失败！");
                continue;
            }

            /* 非法按键：行内提示，不重渲染，不等待"任意键" */
            puts("  ← 无效按键！");
            continue;
        }

        BoardMoveResult result = board_move(&board, direction);
        if (result == BOARD_MOVE_NO_CHANGE)
        {
            /* 此方向无变化：行内提示，不重渲染，不等待"任意键" */
            puts("  ← 此方向无法移动");
            continue;
        }

        if (result == BOARD_MOVE_WIN && !reached_2048)
            reached_2048 = true;

        /* 生成新方块 */
        if (!board_spawn_tile(&board))
        {
            /* 棋盘已满，理论上 board_is_game_over 会捕获，兜底 */
        }

        /* 成功移动后才重渲染 */
        game_show_screen(&board, current_user);

        if (board_is_game_over(&board))
        {
            puts("");
            puts("无可移动方块，游戏结束！");
            break;
        }
    }

    /* 4) 正常退出 / 失败退出 / Q 退出：若玩家没在本局手动存过档，
           则保留一局"自然结束前的最后进度"作为下次开局可恢复的存档。
           真正"通关 / 死亡"后才清理存档。 */
    bool game_finished = reached_2048 || board_is_game_over(&board);

    if (game_finished)
    {
        /* 本局已结束：写入排行榜最高分，删除存档 */
        if (rank_save_score(SCORES_DATA_FILE, current_user, board.score))
        {
            /* 保存成功 */
        }
        (void)save_delete(SAVES_DATA_FILE, current_user);
    }
    else
    {
        /* 玩家按 Q 主动放弃：自动把当前进度写为存档，方便下次继续 */
        if (save_persist(SAVES_DATA_FILE, current_user, &board))
        {
            /* 静默保存，不在结束界面额外提示 */
        }
    }

    game_show_over(&board, reached_2048);
}
