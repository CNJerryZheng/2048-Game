# 2048 Game (Qt)

基于 C 语言游戏核心与 Qt 6 Widgets 界面的 2048 游戏。

## 功能

- 注册、登录、游客模式与用户中心
- 自动保存、加密存档、撤销与重新开始
- 排行榜、历史成绩、最大方块、步数和用时统计
- 可配置自动保存步数
- 预留游戏模式与 MOD 注册接口

## 构建环境

- CMake 3.16 或更高版本
- Qt 6 Widgets
- 支持 C11 与 C++17 的编译器

## 构建

```bash
cmake -S . -B build -DCMAKE_PREFIX_PATH=/path/to/Qt
cmake --build build
```

Windows 下生成的程序默认位于 `build/bin/2048_game_qt.exe`。

原控制台版本保存在仓库的 `noqt` 分支。
