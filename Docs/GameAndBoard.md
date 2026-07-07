# `game` & `board` 模块接口文档

> 本文档聚焦项目里**最早存在的两个核心模块**：`game`（对局主循环）和 `board`（棋盘数据 / 渲染 / 规则）。
>
> 本次会话的存档系统（`save`）也调用了 `board` 的接口，但此处**不展开存档**，仅在"调用关系"中点到为止。
> 存档细节见 [`Docs/API.md`](./API.md)。

## 目录

- [1. 模块边界](#1-模块边界)
- [2. 数据结构](#2-数据结构)
  - [2.1 `Board`](#21-board)
  - [2.2 `BoardDirection`](#22-boarddirection)
  - [2.3 `BoardMoveResult`](#23-boardmoveresult)
- [3. `board.h` 公开接口](#3-boardh-公开接口)
  - [3.1 `board_initialize`](#31-board_initialize)
  - [3.2 `board_render`](#32-board_render)
  - [3.3 `board_spawn_tile`](#33-board_spawn_tile)
  - [3.4 `board_move`](#34-board_move)
  - [3.5 `board_is_game_over`](#35-board_is_game_over)
  - [3.6 `board_get_max_tile`](#36-board_get_max_tile)
- [4. `board.c` 内部算法要点](#4-boardc-内部算法要点)
  - [4.1 新方块生成](#41-新方块生成)
  - [4.2 单行移动 / 合并](#42-单行移动--合并)
  - [4.3 四个方向归一到"向左靠拢"](#43-四个方向归一到向左靠拢)
  - [4.4 ANSI 配色与打印](#44-ansi-配色与打印)
  - [4.5 随机种子懒初始化](#45-随机种子懒初始化)
- [5. `game.h` 公开接口](#5-gameh-公开接口)
  - [5.1 `game_run`](#51-game_run)
- [6. `game.c` 内部状态机](#6-gamec-内部状态机)
  - [6.1 主循环结构](#61-主循环结构)
  - [6.2 键位与解析](#62-键位与解析)
  - [6.3 重渲染策略](#63-重渲染策略)
  - [6.4 退出后处理（与存档 / 排行榜的交互）](#64-退出后处理与存档--排行榜的交互)
- [7. `game.c` 内部辅助函数](#7-gamec-内部辅助函数)
- [8. 行为约束与已知限制](#8-行为约束与已知限制)

---

## 1. 模块边界

```text
                ┌──────────────┐
                │  main / user │
                └──────┬───────┘
                       │ game_run(current_user)
                       ▼
                ┌──────────────┐    board_initialize / render /
                │    game      │←── board_move / spawn / is_game_over /
                │  (主循环)    │    board_get_max_tile
                └──────┬───────┘
                       │
                       │ 调 ui_clear_screen / board_render / save_*
                       ▼
                ┌──────────────┐
                │    board     │
                │ (数据/渲染)  │
                └──────────────┘
```

- **`board`** 是**无状态、无 I/O** 的纯逻辑模块（除了 `board_render` 会向 stdout 写 ANSI 输出），不依赖任何其它业务模块。
- **`game`** 是**有状态、有 I/O** 的对局主循环，依赖 `board` / `ui` / `save` / `rank` / `config`。
- **`user` / `main`** 只调用 `game_run`，从不直接调 `board_*` 函数。

---

## 2. 数据结构

### 2.1 `Board`

```c
typedef struct {
    int  tiles[BOARD_SIZE][BOARD_SIZE];   // 0 = 空；非 0 = 方块数值（2、4、8、…）
    int  score;                            // 当前对局分数
    bool won;                              // 是否曾达到 2048
} Board;
```

| 字段 | 取值范围 | 含义 |
|---|---|---|
| `tiles[r][c]` | `0` 或 `2^k`（k≥1） | 棋盘格子；`0` 表示空格 |
| `score` | `≥ 0` | 单局累计分数；每次合并出新方块时加上该方块数值 |
| `won` | `true` / `false` | 任意一次合并产生 2048 后置 `true`，永不清回 `false` |

> **生命周期**：`board` 由 `game_run` 在栈上声明；其它模块只能通过 `Board*` 引用。

### 2.2 `BoardDirection`

```c
typedef enum {
    BOARD_DIR_UP = 0,
    BOARD_DIR_DOWN,
    BOARD_DIR_LEFT,
    BOARD_DIR_RIGHT
} BoardDirection;
```

`board_move` 的方向参数；`game.c::game_parse_key` 负责把键位翻译成这个枚举。

### 2.3 `BoardMoveResult`

```c
typedef enum {
    BOARD_MOVE_NO_CHANGE = 0,   // 此方向无任何方块可移动 / 合并
    BOARD_MOVE_OK,              // 移动或合并成功，已生成新方块
    BOARD_MOVE_WIN              // 同 OK，并且本步合并出了 2048
} BoardMoveResult;
```

> `board_move` 返回值只反映**当次调用**的胜负；`Board.won` 是历史累计标志，二者不重复。

---

## 3. `board.h` 公开接口

### 3.1 `board_initialize`

```c
void board_initialize(Board *board);
```

| 项 | 说明 |
|---|---|
| 入参 | `board`：必须为非 `NULL` 且指向合法 `Board` |
| 返回值 | 无 |
| 副作用 | 清空 `tiles` 与 `score`；`won = false`；**生成两个起始方块**；首次调用会 `srand((unsigned)time(NULL))` |
| 失败模式 | `board == NULL` 时无操作 |

**起始方块规则**：

- 共生成 **2** 个；每次按 `GAME_NEW_TILE_TWO_PROBABILITY` 决定 2（默认 90%）或 4（10%）。
- 必须**不重叠**——在 `board_count_empty` 后的随机位置上放置。

### 3.2 `board_render`

```c
void board_render(const Board *board);
```

| 项 | 说明 |
|---|---|
| 入参 | `board`：只读 |
| 副作用 | 向 stdout 写入棋盘 ASCII 框 + 数字；每格使用 `board_tile_ansi` 给出的 ANSI 前景 / 背景色 |
| 失败模式 | `board == NULL` 时无操作 |

**输出结构**（以 4×4 为例）：

```text
+--------+--------+--------+--------+
| 1024   |    4   |    8   |    2   |
+--------+--------+--------+--------+
|   16   |  128   |    2   |        |
+--------+--------+--------+--------+
|    2   |   32   |  256   |   64   |
+--------+--------+--------+--------+
|    4   |    8   |   16   |    2   |
+--------+--------+--------+--------+
```

- 单格宽度 = `BOARD_CELL_MAX`（5）；含两字符的左 / 右填充。
- 数值列使用 `snprintf("%d", ...)` 转字符串，再 `printf("%*s", ...)` 右对齐。
- `0` 显示为空格，但仍带 ANSI 背景色。

### 3.3 `board_spawn_tile`

```c
bool board_spawn_tile(Board *board);
```

| 项 | 说明 |
|---|---|
| 副作用 | 在一个随机空格放置 2（90%）或 4（10%） |
| 返回值 | `true` = 成功生成；`false` = 棋盘已满或 `board == NULL` |

> **不保证 spawn 顺序**：因为每次掷一次随机骰子决定值，所以多局同种子结果不同（`srand` 在 `board_initialize` 首次调用时才执行）。

### 3.4 `board_move`

```c
BoardMoveResult board_move(Board *board, BoardDirection direction);
```

| 项 | 说明 |
|---|---|
| 入参 | `board`：可变；`direction`：四个方向之一 |
| 副作用 | 若发生移动 / 合并：更新 `tiles`、把 `score_gain` 累加到 `board->score`；若本步产生 2048，置 `board->won = true` |
| 返回值 | 见 [`BoardMoveResult`](#23-boardmoveresult) |
| 失败模式 | `board == NULL` → 返回 `BOARD_MOVE_NO_CHANGE`，无副作用 |

> **关键约定**：`board_move` 本身**不会**自动 spawn 新方块。spawn 由 `game_run` 在收到 `BOARD_MOVE_OK` / `BOARD_MOVE_WIN` 之后**显式调用** `board_spawn_tile`。
>
> 也就是说"移动一次"在外部逻辑上等于：
>
> ```c
> BoardMoveResult r = board_move(&board, dir);
> if (r != BOARD_MOVE_NO_CHANGE) board_spawn_tile(&board);
> ```

### 3.5 `board_is_game_over`

```c
bool board_is_game_over(const Board *board);
```

| 项 | 说明 |
|---|---|
| 返回值 | `true` = 既无空格又无任意相邻可合并方块；`false` = 还有操作空间 |
| 失败模式 | `board == NULL` → 返回 `true`（防御性写法） |

**判断顺序**（与实现一致）：

1. 扫描全表，若任一格为 `0` → `false`（还有空格）。
2. 横向相邻：若 `tiles[r][c] == tiles[r][c+1]` → `false`。
3. 纵向相邻：若 `tiles[r][c] == tiles[r+1][c]` → `false`。
4. 上述皆否 → `true`。

### 3.6 `board_get_max_tile`

```c
void board_get_max_tile(const Board *board, int *max_tile);
```

| 项 | 说明 |
|---|---|
| 出参 | `*max_tile` 被设置为棋盘最大值（全部为 0 时为 0） |
| 失败模式 | `max_tile == NULL` 或 `board == NULL` → 无操作 |

> **复杂度** O(`BOARD_SIZE²`)。`game.c` 仅在 `game_show_header` 中调用一次，开销可忽略。

---

## 4. `board.c` 内部算法要点

> `board.c` 有四个 `static` 辅助函数（不导出）：`board_random_index` / `board_count_empty` / `board_spawn_tile_internal` / `board_process_line` / `board_print_*` / `board_tile_ansi`。这里给出关键设计点。

### 4.1 新方块生成

```text
empty_count = board_count_empty(board)
if empty_count == 0: return false
target      = rand() % empty_count
遍历所有格，遇到第 target 个空格时写入
```

- `board_spawn_tile` 仅决定"值是 2 还是 4"；"放哪一格"由 `board_spawn_tile_internal` 处理。
- 概率：`rand() % 100 < GAME_NEW_TILE_TWO_PROBABILITY` → 2，否则 4。

### 4.2 单行移动 / 合并

`board_process_line` 是**核心算法**：对一维 `int[BOARD_SIZE]` 做"压缩 + 合并 + 补 0"。

```text
输入：   [0, 2, 2, 4]
压缩：   [2, 2, 4, 0]      // 去掉空格
合并：   [4, 4, 0, 0]      // 2+2=4，4 不与 0 合并
回填：   [4, 4, 0, 0]
```

**合并规则**：

- 仅合并**相邻且相同**的两个方块。
- 一次扫描中，每对方块只合并一次（`idx += 2`）。
- **不连续合并**：例如 `[2, 2, 4, 4]` → `[4, 8, 0, 0]`，**不是** `[16, 0, 0, 0]`。
- 本步合并出新方块时：累加到 `*score_gain`；若新方块为 2048，置 `*reached_2048 = true`。
- `changed` 仅在回填前后 `line_values` 不同时返回 `true`（调用方据此决定是否计入 `any_change`）。

### 4.3 四个方向归一到"向左靠拢"

`board_move` 的实现是**把 UP / RIGHT / DOWN 三个方向都反转成 LEFT**：

| 实际方向 | 处理 |
|---|---|
| `LEFT` | 直接拷贝行 → `board_process_line` → 写回 |
| `RIGHT` | 反转行 → `board_process_line`（按 LEFT 处理）→ 反转回来 → 写回 |
| `UP` | 拷贝列 → `board_process_line`（按 LEFT 处理）→ 写回 |
| `DOWN` | 反转列 → `board_process_line`（按 LEFT 处理）→ 反转回来 → 写回 |

> 这意味着 `board_process_line` 的语义固定为"所有非 0 方块向 `line[0]` 方向靠拢"。
> 优点：消除 4 份重复逻辑。代价：每次反向 / 写回时多一倍 `BOARD_SIZE / 2` 的 swap，可忽略。

### 4.4 ANSI 配色与打印

- `board_tile_ansi(v)`：按 v 的具体值返回 ANSI 转义序列（前景 + 背景）。`default` 走洋红底白字，覆盖 2048 之后的更大值。
- 边框 / 分割线：纯 ASCII `-` `+` `|`，**不带颜色**。
- `board_print_row` 用 `printf(" %s%*s" BOARD_ANSI_RESET " |", ansi, BOARD_CELL_MAX, text)`，每格独立着色后立即 reset，**避免**颜色"溢出"到框线。

### 4.5 随机种子懒初始化

```c
static bool seed_initialized = false;
if (!seed_initialized) {
    srand((unsigned)time(NULL));
    seed_initialized = true;
}
```

- `static` 局部变量 → 进程内只初始化一次。
- 放在 `board_initialize` 中，**不**放进 `board_spawn_tile`，避免每次 spawn 都重新 `time()`。
- 注意：因为是 `static`，如果项目后续引入多线程访问 `board_*`，这个种子初始化本身没问题，但 `rand()` 不再线程安全——目前项目为单线程，不构成问题。

---

## 5. `game.h` 公开接口

### 5.1 `game_run`

```c
void game_run(const char *current_user);
```

| 项 | 说明 |
|---|---|
| 入参 | `current_user`：已登录用户；为 `NULL` 或 `""` 时直接提示并返回 |
| 返回值 | 无 |
| 副作用 | 整局对局的所有屏幕输出、可能的存档读写（`Data/saves.txt`）、可能的排行榜写入（`Data/scores.txt`） |
| 调用方 | `Src/user.c::run_game_system` 主菜单的 `2` / `4` 分支 |

**核心状态**（全部为 `game_run` 局部）：

| 局部变量 | 用途 |
|---|---|
| `Board board` | 当前棋盘 |
| `bool reached_2048` | 本局是否达成 2048（用于结束语） |
| `bool loaded_from_save` | 本局是否从存档恢复（控制是否走 `board_initialize`） |

---

## 6. `game.c` 内部状态机

### 6.1 主循环结构

`game_run` 的执行步骤（按代码顺序）：

1. **空用户检查**：`current_user` 为 `NULL` / `""` → 打印 `请先登录后再开始游戏！` → `ui_wait_for_enter` → 返回。
2. **存档检测与续局**（详见 [`API.md` §10](./API.md#10-gameh-对局主循环)）：
   - `save_exists` 为真 → `save_load` 拿到 `preview` → `ui_ask_continue_save`
   - 玩家选"继续" → `board = preview`，`reached_2048 = board.won`，`loaded_from_save = true`
   - 玩家选"重新开始" → `save_delete` 清档
3. **新局**：`!loaded_from_save` → `board_initialize(&board)`。
4. **首次渲染**：`game_show_screen`（清屏 + 标题 + 玩家信息 + 棋盘 + 帮助）。
5. **主循环**（详见下表）。
6. **退出后处理**（见 [6.4](#64-退出后处理与存档--排行榜的交互)）。
7. **`game_show_over`**：渲染结束界面并等待任意键。

### 6.2 键位与解析

`game_run` 内部的处理顺序（**完整优先级**）：

| # | 条件 | 处理 |
|---|---|---|
| 1 | `key == '\n' \|\| key == '\r'` | `continue`（忽略回车） |
| 2 | `game_parse_key` 返回 `false` 且 `*quit == true`（即 Q） | `break` 退出主循环 |
| 3 | `key == 'p' \|\| key == 'P'` | `save_persist(SAVES_DATA_FILE, current_user, &board)`；成功 → `ui_show_save_success(board.score)`；失败 → 行内 `存档失败！` |
| 4 | `game_parse_key` 返回 `false` 且非 Q | 行内 `  ← 无效按键！` |
| 5 | `game_parse_key` 返回 `true`（解析出方向） | 进入"移动"分支 |
| 5a | `board_move` 返回 `BOARD_MOVE_NO_CHANGE` | 行内 `  ← 此方向无法移动` |
| 5b | 成功 | `board_spawn_tile` → `game_show_screen` 重渲染 → 检查 `board_is_game_over` |

**`game_parse_key` 支持的输入**（大小写不敏感）：

| 键 | 方向 |
|---|---|
| `W` | UP |
| `S` | DOWN |
| `A` | LEFT |
| `D` | RIGHT |
| `Q` | （不返回方向，仅设 `*quit = true`） |
| `ESC` + `[` + `A/B/C/D` | UP / DOWN / RIGHT / LEFT（方向键） |
| 其它 | 返回 `false`，`*quit = false`（由调用方走"非法键"路径） |

> **注意**：`P` 不在 `game_parse_key` 中识别，而是在主循环里直接 `key == 'p' || 'P'` 分支处理。
> 这样设计的好处是 `P` 的处理逻辑（写存档）能直接拿到 `board` 上下文，不必改 `game_parse_key` 的签名。

**`game_read_key` 跨平台差异**：

| 平台 | 行为 |
|---|---|
| Windows (`_WIN32`) | `_getch()` 直接读一次按键，无需回车 |
| Unix | `getchar()` 阻塞读一字符；把 `\r` 规整为 `\n`（与主循环对回车的处理对齐） |

### 6.3 重渲染策略

> 这是 `game_run` 当前的关键设计点。

| 触发 | 是否重渲染 |
|---|---|
| 进入游戏 / 续局后 | **是**（首次 `game_show_screen`） |
| 移动成功（`BOARD_MOVE_OK` / `BOARD_MOVE_WIN`） | **是**（`game_show_screen`） |
| 移动失败（`BOARD_MOVE_NO_CHANGE`） | 否（行内提示） |
| 非法键 | 否（行内提示） |
| `P` 存档成功 | 否（行内提示） |
| 棋盘死亡 | 否（额外打印 `无可移动方块，游戏结束！` 然后 break） |

**设计意图**：

- "按键即移动"的直接操作感：成功移动 → 立即看到新棋盘。
- 失败 / 非法输入 → 不刷屏，不强制"按任意键继续"，让光标原地提示即可。
- 退出 / 死亡 → 由 `game_show_over` 完整清屏重绘。

### 6.4 退出后处理（与存档 / 排行榜的交互）

主循环 `break` 后：

```c
bool game_finished = reached_2048 || board_is_game_over(&board);

if (game_finished) {
    // 写排行榜最高分
    rank_save_score(SCORES_DATA_FILE, current_user, board.score);
    // 清掉存档（防止下次进来时还能"继续"已死的对局）
    save_delete(SAVES_DATA_FILE, current_user);
} else {
    // 玩家按 Q 主动退出：静默保存当前进度
    save_persist(SAVES_DATA_FILE, current_user, &board);
}

game_show_over(&board, reached_2048);
```

| 退出原因 | `game_finished` | 行为 |
|---|---|---|
| 棋盘死亡 | `true` | 入榜 + 清档 |
| 达到 2048 后继续玩到死 | `true` | 入榜 + 清档 |
| 按 Q 主动放弃 | `false` | 静默存档（主菜单下次进入可用"4. 继续上局"） |
| 异常分支 / `board_move` 失败 | `false` | 同 Q 路径 |

> **细节**：`board_is_game_over(&board)` 在循环退出**之后**再次调用，与循环内最后那次调用的语义一致（棋盘状态未变）。用于替代维护一个 `bool died_in_loop` 局部变量。

---

## 7. `game.c` 内部辅助函数

| 函数 | 签名 | 用途 |
|---|---|---|
| `game_show_header` | `static void (const Board *b, const char *user)` | 打印"玩家 / 当前分数 / 最大方块 / 分隔线" |
| `game_show_help` | `static void (void)` | 打印操作说明（W/S/A/D/P/Q） |
| `game_show_screen` | `static void (const Board *b, const char *user)` | 清屏 + 标题 + 头 + 棋盘 + 帮助（一帧完整画面） |
| `game_wait_for_key` | `static void (void)` | 提示"按任意键继续..."并阻塞；Win 用 `_getch`，其他用 `getchar` + 排空行尾 |
| `game_parse_key` | `static bool (int key, BoardDirection *dir, bool *quit)` | 把单个键翻译成方向或退出信号 |
| `game_read_key` | `static int (void)` | 跨平台读一次按键 |
| `game_show_over` | `static void (const Board *b, bool reached_2048)` | 结束画面 + `game_wait_for_key` |

> 这些函数都 `static`，对模块外不可见。如要扩展（例如换主题、改提示文本），改 `game.c` 即可。

---

## 8. 行为约束与已知限制

| 主题 | 约束 |
|---|---|
| 线程安全 | `Board` 不含锁；`board_*` 假设单线程访问 |
| 最大方块 | `board_tile_ansi` 用 `switch` 覆盖到 2048；更大值走 `default`（洋红底），数值仍可正常存储与合并 |
| 棋盘尺寸 | `BOARD_SIZE` 是编译期常量，宏定义在 `config.h`；当前为 4。要改 5×5 / 6×6 仅需改宏 |
| 起始方块 | `board_initialize` 固定生成 2 个；不暴露"自定义起始数量"接口 |
| 合并规则 | "每个方块一帧最多合并一次"——这是 2048 通用规则，**不会**出现 `[2,2,4,4] → 16` |
| 存档兼容性 | `Data/saves.txt` 的字段数为 19（user + score + won + 16 tile）。`storage_parse_save_line` 严格要求 19 字段全命中，缺一不可 |
| 键位 | 大小写均接受；方向键依赖终端的 ESC 序列；Windows 走 `_getch` 直读，不需回车 |
| 输入流 | `game_read_key` 用 `_getch`（Win）或 `getchar`（其他），与 `utils_read_line`（用 `getchar`）**不冲突**——`utils_read_line` 不会在对局循环中被调用 |
| 失败模式 | `board_move` 在 `board == NULL` 时返回 `NO_CHANGE`；`board_render` 在 `board == NULL` 时无操作。**调用方应始终传非空指针**——这些是防御性兜底，不是正常路径 |

---

## 附录 A：调用关系速查

```text
main()
  └── run_game_system()                           [user.c]
        ├── game_run(current_user)                [game.c]
        │     ├── save_exists / save_load         [save.c]
        │     ├── save_persist / save_delete      [save.c]
        │     ├── board_initialize                [board.c]
        │     ├── board_move ── board_process_line
        │     ├── board_spawn_tile
        │     ├── board_render
        │     ├── board_is_game_over
        │     ├── board_get_max_tile
        │     ├── rank_save_score                 [rank.c]
        │     ├── ui_clear_screen / ui_wait_for_enter / ui_show_save_*
        │     └── game_show_over (static)
        ├── rank_show                              [rank.c]
        └── user_show_interface                    [user.c]
```

## 附录 B：常见改动指南

| 想做的事 | 改哪里 |
|---|---|
| 改棋盘尺寸（如 5×5） | `Inc/config.h` 改 `BOARD_SIZE`；`BOARD_CELL_MAX` 可按需同步调整 |
| 改起始方块数 | `Src/board.c::board_initialize` 末尾追加 `board_spawn_tile` 调用 |
| 改新方块概率 | `Inc/config.h::GAME_NEW_TILE_TWO_PROBABILITY` |
| 加键位（如 Z 撤销） | `Src/game.c::game_run` 在 `key` 分支中新增判断；底层解析若要新键，扩展 `game_parse_key` |
| 改帮助文本 | `Src/game.c::game_show_help` |
| 改结束语 | `Src/game.c::game_show_over` |
| 改配色 | `Src/board.c::board_tile_ansi`（按值返回 ANSI 转义序列） |
| 改边框风格 | `Src/board.c::board_print_top_border` / `board_print_separator` / `board_print_row` |
| 改"无法移动"提示文案 | `Src/game.c` 第 253 行附近 |

## 附录 C：与存档系统的协作

- `game_run` 进入时通过 `save_exists` / `save_load` 决定是否走"续局"路径。
- `game_run` 中 `P` 键直接调 `save_persist`。
- `game_run` 退出时根据"自然结束 vs 主动放弃"决定是 `save_delete` 还是 `save_persist`。

更多存档细节（`SaveRecord` 结构、文件格式、原子替换、容量上限等）见 [`Docs/API.md` §6](./API.md#6-saveh-存档模块--新增)。
