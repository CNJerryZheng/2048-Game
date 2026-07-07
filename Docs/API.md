# 2048-Game 接口技术文档

> 本文档整理本项目当前对外暴露的模块接口，以及会话中新增 / 调整的内容。
> 适用于维护者快速查阅函数契约、模块边界与数据格式。

## 目录

- [1. 项目结构](#1-项目结构)
- [2. 通用约定](#2-通用约定)
- [3. `config.h` 编译期常量](#3-configh-编译期常量)
- [4. `board.h` 棋盘模块](#4-boardh-棋盘模块)
- [5. `storage.h` 存储原语](#5-storageh-存储原语)
- [6. `save.h` 存档模块](#6-saveh-存档模块) ⭐ 新增
- [7. `rank.h` 排行榜模块](#7-rankh-排行榜模块)
- [8. `ui.h` 终端界面模块](#8-uih-终端界面模块)
- [9. `user.h` 用户主循环](#9-userh-用户主循环)
- [10. `game.h` 对局主循环](#10-gameh-对局主循环)
- [11. `utils.h` 通用工具](#11-utilsh-通用工具)
- [12. 数据文件格式](#12-数据文件格式)
- [13. 交互流程与键位](#13-交互流程与键位)

---

## 1. 项目结构

```text
2048-Game/
├── CMakeLists.txt          # 构建脚本
├── README.md               # 项目说明
├── Docs/API.md             # 本接口技术文档
├── Data/                   # 运行时数据
│   ├── user.txt            #   用户名\t密码
│   ├── scores.txt          #   用户名\t最高分
│   └── saves.txt           #   存档（本次新增）
├── Inc/                    # 头文件（接口）
│   ├── board.h
│   ├── config.h
│   ├── game.h
│   ├── main.h
│   ├── rank.h
│   ├── save.h              #   本次新增
│   ├── storage.h
│   ├── ui.h
│   ├── user.h
│   └── utils.h
└── Src/                    # 实现
    ├── board.c
    ├── game.c
    ├── main.c
    ├── rank.c
    ├── save.c              #   本次新增
    ├── storage.c
    ├── ui.c
    ├── user.c
    └── utils.c
```

---

## 2. 通用约定

| 约定 | 说明 |
|---|---|
| 头文件保护 | 全部使用 `#pragma once` |
| 布尔类型 | 统一 `<stdbool.h>` 的 `bool`/`true`/`false` |
| 错误返回 | 函数失败返回 `false` / `NULL` / 0；成功返回 `true` / 非 NULL / 有效值 |
| 输入校验 | 所有公开函数对 `NULL` / 非法入参返回失败，不 panic |
| 平台分支 | 仅 `storage.c` / `game.c` 出现 `_WIN32` 分支；其余模块与平台无关 |
| 数据文件 | 全部位于 `Data/` 目录，路径由 `config.h` 集中宏定义 |
| 原子写入 | 涉及覆盖的写操作采用"写临时文件 + rename"模式，避免半写状态 |

---

## 3. `config.h` 编译期常量

```c
#define USER_DATA_FILE   "Data/user.txt"
#define SCORES_DATA_FILE "Data/scores.txt"
#define SAVES_DATA_FILE  "Data/saves.txt"   // 本次新增
```

| 宏 | 默认值 | 用途 |
|---|---|---|
| `USER_DATA_FILE` | `Data/user.txt` | 用户名 / 密码表 |
| `SCORES_DATA_FILE` | `Data/scores.txt` | 排行榜（按用户去重的最高分） |
| `SAVES_DATA_FILE` | `Data/saves.txt` | **本次新增**：每用户一行的对局存档 |
| `USER_PASSWORD_LENGTH_MAX` / `_MIN` | 21 / 6 | 密码长度（含结束符 / 最小值） |
| `USER_NAME_LENGTH_MAX` / `_MIN` | 32 / 2 | 用户名长度 |
| `INPUT_BUFFER_LENGTH` | 128 | 通用输入行缓冲 |
| `RANK_ENTRIES_MAX` | 256 | 排行榜内存中最多加载条数 |
| `RANK_TOP_COUNT` | 10 | 排行榜展示前 N 名 |
| `BOARD_SIZE` | 4 | 棋盘边长（4×4） |
| `BOARD_CELL_MAX` | 5 | 棋盘单格字符宽度 |
| `GAME_NEW_TILE_TWO_PROBABILITY` | 90 | 新方块为 2 的百分比（其余为 4） |

---

## 4. `board.h` 棋盘模块

### 数据结构

```c
typedef struct {
    int  tiles[BOARD_SIZE][BOARD_SIZE];   // 0 = 空
    int  score;                            // 当前对局分数
    bool won;                              // 是否曾达到 2048
} Board;

typedef enum { BOARD_DIR_UP, BOARD_DIR_DOWN,
               BOARD_DIR_LEFT, BOARD_DIR_RIGHT } BoardDirection;

typedef enum { BOARD_MOVE_NO_CHANGE = 0,
               BOARD_MOVE_OK,
               BOARD_MOVE_WIN } BoardMoveResult;
```

### 函数

| 函数 | 行为 |
|---|---|
| `void board_initialize(Board *board)` | 清空棋盘与分数，并随机生成两个起始方块（首次调用时初始化 `srand`）。`board == NULL` 时无操作。 |
| `void board_render(const Board *board)` | 用 ANSI 着色把棋盘绘制到 stdout。`board == NULL` 时无操作。 |
| `bool board_spawn_tile(Board *board)` | 在随机空格生成 2（90%）或 4（10%）；无空位返回 `false`。 |
| `BoardMoveResult board_move(Board *board, BoardDirection dir)` | 按方向执行一次移动 + 合并。返回值见下表。 |
| `bool board_is_game_over(const Board *board)` | 无空格且任意相邻方块均不同时返回 `true`。 |
| `void board_get_max_tile(const Board *board, int *max_tile)` | 将棋盘最大值写入 `*max_tile`（`max_tile == NULL` 时无操作）。 |

`board_move` 返回值：

| 返回 | 含义 |
|---|---|
| `BOARD_MOVE_NO_CHANGE` | 方向不合法或无方块可移动，棋盘不变 |
| `BOARD_MOVE_OK` | 移动 / 合并成功，已生成新方块 |
| `BOARD_MOVE_WIN` | 同 `OK`，且本步合并出了 2048 |

---

## 5. `storage.h` 存储原语

> 本次新增 `storage_write_save` 与 `storage_parse_save_line` 两个底层原语，供 `save.c` 复用。

```c
FILE *storage_open_read(const char *file_path);     // "r"，不存在返回 NULL
FILE *storage_open_append(const char *file_path);   // "a"，不存在则创建
FILE *storage_open_write(const char *file_path);     // "w"，不存在则创建
```

| 函数 | 行为 |
|---|---|
| `bool storage_read_line(FILE *f, char *buf, size_t n)` | 用 `fgets` 读一行；参数非法返回 `false`。 |
| `bool storage_write_user(FILE *f, const char *u, const char *p)` | 写 `"%s\t%s\n"`。 |
| `bool storage_write_score(FILE *f, const char *u, int s)` | 写 `"%s\t%d\n"`（`s < 0` 失败）。 |
| **`bool storage_write_save(FILE *f, const char *u, int score, int won, const int tiles[4][4])`** | 写存档行：`"%s\t%d\t%d\t16 个 int\n"`（`won` 取 0/1）。 |
| **`bool storage_parse_save_line(const char *line, char *u, size_t un, int *score, int *won, int tiles[4][4])`** | 解析一行存档；要求 19 个字段（user + score + won + 16 个 tile）全部命中，否则返回 `false`。 |
| `bool storage_close(FILE *f)` | `fclose` 包装。 |
| `bool storage_remove_file(const char *p)` | `remove()` 包装。 |
| `bool storage_replace_file(const char *src, const char *dst)` | 跨平台原子替换：Win 用 `MoveFileExA(REPLACE_EXISTING | WRITE_THROUGH)`，其他用 `rename()`。 |

新增原语的格式约定见 [§12 数据文件格式](#12-数据文件格式)。

---

## 6. `save.h` 存档模块 ⭐ 新增

> 用户级进度存档。**和排行榜分离**：存档是"对局进度"，排行榜是"历史最高分"。

### 数据结构

```c
typedef struct {
    char username[USER_NAME_LENGTH_MAX];
    int  score;
    bool won;
    int  tiles[BOARD_SIZE][BOARD_SIZE];
} SaveRecord;
```

### 函数

| 函数 | 行为 |
|---|---|
| `bool save_exists(const char *saves_file, const char *username)` | 指定用户是否存在有效存档。`username` 为空或 `NULL` 返回 `false`；文件不存在返回 `false`。 |
| `bool save_load(const char *saves_file, const char *username, Board *out)` | 把指定用户存档装入 `out`（同时也写 `score`、`won`）。`out` 允许为 `NULL`，此时仅做"存在性"检查。 |
| `bool save_persist(const char *saves_file, const char *username, const Board *board)` | 写入 / 覆盖指定用户的存档。**采用"全量读 → 内存改 → 临时文件 → rename"原子替换**。返回 `false` 的情形：参数非法、临时文件数量超 `RANK_ENTRIES_MAX`、I/O 失败。 |
| `bool save_delete(const char *saves_file, const char *username)` | 删除指定用户的存档。无匹配项时返回 `false`（视为"无操作"），不报错。 |

### 容量与冲突

- 内存中以 `SaveRecord[RANK_ENTRIES_MAX]` 数组承载（与排行榜共用同一个上限 = 256）。`save_persist` 满时直接放弃，**不覆盖**任何已存在记录。
- 同一 `username` 只保留**一条**记录。多次 `save_persist` 会覆盖前一次。

### 失败回滚

- `save_persist` / `save_delete` 任一阶段失败，都会 `storage_remove_file(cache_file)` 清理临时文件，不会留下半写状态。

### 调用方约定

| 调用方 | 行为 |
|---|---|
| `game.c::game_run` 进入时 | 用 `save_exists` 判断要不要问"继续上局" |
| `game.c::game_run` 玩家选"继续" | 用 `save_load(..., &board)` 直接覆盖 `board` |
| `game.c::game_run` 玩家按 `P` | 用 `save_persist(..., &board)` 立即落盘 |
| `game.c::game_run` 玩家选"重新开始" / 自然通关 | `save_delete` 清理 |
| `user.c::run_game_system` 主菜单 | `save_exists` 决定是否显示"4. 继续上局" |

---

## 7. `rank.h` 排行榜模块

> 与 `save.h` 解耦：排行榜是历史战绩（按用户去重的最高分），存档是当前对局进度。

```c
typedef struct {
    char username[USER_NAME_LENGTH_MAX];
    int  score;
} RankEntry;
```

| 函数 | 行为 |
|---|---|
| `void rank_show(const char *scores_file, const char *current_user)` | 读取 → 去重 → 降序 → 调 `ui_show_ranking` 渲染。 |
| `bool rank_save_score(const char *scores_file, const char *u, int s)` | 追加 / 更新用户最高分；同样采用原子替换。 |

---

## 8. `ui.h` 终端界面模块

> 本次新增 3 个对外函数；并修改 `ui_show_main_menu` 签名。

### 修改：`ui_show_main_menu` 新签名

```c
void ui_show_main_menu(const char *current_user, bool has_save);
```

| 参数 | 含义 |
|---|---|
| `current_user` | 当前登录用户；未登录传 `NULL` / `""` |
| `has_save` | 当前用户是否存在可继续的存档；**仅当登录且 `has_save == true` 时**才在菜单中显示"4. 继续上局" |

> 旧签名（`bool has_save` 缺失）已废弃，调用方需同步更新。

### 新增

| 函数 | 行为 |
|---|---|
| `bool ui_ask_continue_save(int saved_score, bool saved_won)` | 清屏 → 打印"检测到上局存档"和分数 / 是否已达 2048 → 循环读 `1/2`。返回 `true` = 继续，`false` = 重新开始。 |
| `void ui_show_save_prompt(void)` | 打印 `P  保存当前进度（不退出游戏）`，已并入 `game.c::game_show_help` 输出。 |
| `void ui_show_save_success(int saved_score)` | 行内提示 `  ← 存档成功！当前进度已保存（分数 %d）。` |

### 未变动的接口

```c
void ui_initialize_console(void);
void ui_show_welcome(void);
void ui_show_user_menu(const char *current_user);
void ui_show_register_page(void);
void ui_show_login_page(void);
void ui_show_game(const char *current_user);
void ui_show_ranking(const RankEntry *entries, int count,
                     const char *current_user, int user_rank, int user_best);
void ui_clear_screen(void);
void ui_wait_for_enter(void);
```

---

## 9. `user.h` 用户主循环

```c
void run_game_system(void);
```

> 由 `main()` 直接调用。实现位于 `Src/user.c`，是项目的菜单 / 主循环所在。

### 主菜单分支

| 输入 | 行为 |
|---|---|
| `1` | 进入用户中心（登录 / 注册 / 注销） |
| `2` | **未登录**提示先登录；**已登录**调用 `game_run` |
| `3` | 调 `rank_show` 渲染排行榜 |
| `4` | **未登录**提示先登录；**已登录 + 无存档**提示"没有可继续的存档"；否则调 `game_run`（`game_run` 内部仍会再次询问"继续 / 重新开始"） |
| `0` | 退出程序 |
| 其它 | "无效选项，请重新输入" |

> "4. 继续上局"仅在登录且 `save_exists(...) == true` 时由 `ui_show_main_menu` 渲染；用户手动输入 `4` 在其它状态下会被 `user.c` 兜底拒绝。

---

## 10. `game.h` 对局主循环

```c
void game_run(const char *current_user);
```

> `current_user` 必为已登录用户。

### 流程概要（本次重写）

1. **空用户检查**：`current_user` 空 → 提示"请先登录" → 等待回车 → 返回。
2. **存档检测与续局**：
   - `save_exists(SAVES_DATA_FILE, current_user)` 为真 → `ui_ask_continue_save`
   - 玩家选"继续" → `save_load(...)` 装入 `board`，`reached_2048 = board.won`，`loaded_from_save = true`
   - 玩家选"重新开始" → `save_delete(...)` 清理旧存档
3. **新局**：`!loaded_from_save` → `board_initialize(&board)`。
4. **首次渲染**：`game_show_screen` 渲染一次棋盘（玩家信息、分数、棋盘、帮助）。
5. **主循环**：
   - 打印 `请按方向键或 W/A/S/D 操作（Q 退出 / P 存档）：` → 读单键。
   - 解析键（`game_parse_key`）：
     | 键 | 处理 |
     |---|---|
     | `\n` / `\r` | 忽略 |
     | `W/A/S/D` / 方向键 | 调 `board_move`；成功才重渲染 |
     | `P` | 调 `save_persist` 写盘 + 行内提示 |
     | `Q` | 跳出循环 |
     | 其它 | 行内提示 `  ← 无效按键！` |
   - 移动返回 `BOARD_MOVE_NO_CHANGE` → 行内提示 `  ← 此方向无法移动`，**不重渲染、不等待"任意键"**。
   - 移动成功 → 生成新方块 → `game_show_screen` 重渲染 → 死亡则跳出。
6. **退出后处理**：
   - `reached_2048` 或 `board_is_game_over` → `rank_save_score` + `save_delete`
   - 玩家按 `Q` 主动放弃 → 静默 `save_persist` 保留当前进度
7. **`game_show_over`** 展示结束界面。

### 行为约定（重要）

- **不重渲染**：非法键 / 无法移动的方向 / 按 `P` 后均不重渲染。
- **不强制等待"任意键继续"**：行内提示后直接回到循环顶继续等键。
- **退出条件**：玩家按 `Q`、棋盘死亡、达到 2048 后玩家继续到死亡。

---

## 11. `utils.h` 通用工具

| 函数 | 行为 |
|---|---|
| `bool utils_string_equal(const char *a, const char *b)` | 串值相等；任一为 `NULL` 返回 `false`。 |
| `bool utils_read_line(const char *prompt, char *buf, size_t n)` | 打印 `prompt` → 读一行到 `buf`；溢出或 `EOF`（且无内容）返回 `false`。 |
| `bool utils_read_choice(char *choice)` | `read_line` 包装；要求输入恰好 1 字符。 |
| `void utils_copy_string(char *dst, const char *src, size_t n)` | 限长拷贝；恒写终止符。 |
| `bool utils_is_valid_username(char c)` | `[0-9A-Za-z_]`。 |
| `bool utils_is_valid_password(char c)` | 不可为空格 / `\t` / 换行。 |

---

## 12. 数据文件格式

### `Data/user.txt`

```
<username>\t<password>\n
```

### `Data/scores.txt`

```
<username>\t<score>\n
```

> 实际写入时采用"全量重写 + rename 原子替换"，已用 `rank_deduplicate` 按用户去重，保留最高分。

### `Data/saves.txt` ⭐ 新增

```
<username>\t<score>\t<won_flag>\t<t0>\t<t1>\t...\t<t15>\n
```

| 字段 | 类型 | 说明 |
|---|---|---|
| `username` | 字符串 | 最长 31 字符 |
| `score` | `int` | 当前对局分数 |
| `won_flag` | `0` / `1` | 是否曾达 2048 |
| `t0` … `t15` | `int` | 棋盘 4×4 按行展开；`0` 表示空格 |

**示例**：

```
alice	1234	1	2	0	4	0	0	0	0	0	0	0	0	0	2	4	0	0
bob	512	0	2	2	0	0	0	4	0	0	0	0	0	0	0	0	0	0
```

**约束**：

- 同一 `username` 至多一条记录；新写入会覆盖旧记录。
- 解析要求 19 个字段（user + score + won + 16 tile）全部命中，否则整行被忽略。
- `score` 不可为负，否则忽略该行。

---

## 13. 交互流程与键位

### 主菜单

| 输入 | 行为 |
|---|---|
| `1` | 用户中心 |
| `2` | 开始游戏（已登录则直接进入；未登录提示先登录） |
| `3` | 查看排行榜 |
| `4` | 继续上局（仅当登录且存在存档时显示） |
| `0` | 退出 |

### 用户中心

| 状态 | 输入 | 行为 |
|---|---|---|
| 未登录 | `1` | 登录 |
| 未登录 | `2` | 注册 |
| 未登录 | `0` | 返回主菜单 |
| 已登录 | `1` | 注销 |
| 已登录 | `0` | 返回主菜单 |

### 对局中

| 输入 | 行为 |
|---|---|
| `W` / `↑` | 向上合并 |
| `S` / `↓` | 向下合并 |
| `A` / `←` | 向左合并 |
| `D` / `→` | 向右合并 |
| `P` | 立即保存当前进度（不退出） |
| `Q` | 退出本局并返回主菜单；进度自动保存为存档 |
| 回车 | 忽略（不重渲染） |
| 其它 | 行内提示 `  ← 无效按键！` |

### 进入对局（含存档时）

| 存档状态 | 选项 | 行为 |
|---|---|---|
| 有存档 | `1` | 继续上局 → `save_load` 装入 `Board` |
| 有存档 | `2` | 重新开始 → `save_delete` 清档 → 新局 |
| 无存档 | — | 直接新局 |

### 退出对局

| 结束原因 | 行为 |
|---|---|
| 玩家按 `Q` 主动放弃 | `save_persist` 静默保留进度，方便"4. 继续上局" |
| 棋盘死亡 / 达到 2048 | `rank_save_score` 入榜 + `save_delete` 清档 |

---

## 附录 A：本会话变更清单

| 类型 | 路径 | 变更 |
|---|---|---|
| 新增 | `Inc/save.h` | 存档模块接口 |
| 新增 | `Src/save.c` | 存档模块实现（全量读 / 原子写） |
| 修改 | `Inc/config.h` | 加 `SAVES_DATA_FILE` 宏 |
| 修改 | `Inc/storage.h` | 加 `storage_write_save` / `storage_parse_save_line` 原型；显式 `#include "config.h"` |
| 修改 | `Src/storage.c` | 实现新增原语；显式 `#include "utils.h"` |
| 修改 | `Inc/ui.h` | 加 `ui_ask_continue_save` / `ui_show_save_prompt` / `ui_show_save_success`；`ui_show_main_menu` 增加 `has_save` 参数 |
| 修改 | `Src/ui.c` | 实现新增界面；显式 `#include "utils.h"` / `<stdbool.h>` |
| 修改 | `Src/game.c` | `game_run` 整合存档：进入时询问续局；按 `P` 立即存档；Q 退出自动存档；通关 / 死亡清档 + 入榜 |
| 修改 | `Src/user.c` | 主菜单增 `choice == '4'` 分支；调用 `ui_show_main_menu` 前用 `save_exists` 计算 `has_save` |
| 修改 | `CMakeLists.txt` | `add_executable` 列表中加入 `Src/save.c` |
| 文档 | `README.md` | （未改动，可按需补充存档章节） |
| 新增 | `Docs/API.md` | 本文档 |

## 附录 B：编译验证

```bash
cmake -S . -B build
cmake --build build
```

> 当前在本机（macOS / Homebrew GCC 16）实测编译通过，无 warning。
