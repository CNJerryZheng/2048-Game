# 2048 Game

基于 C、CMake、Ninja 和 MinGW 构建的控制台 2048 游戏项目。

## 小组开发环境

VS Code 配置不包含任何成员电脑的绝对路径。每位成员需要设置以下用户环境变量：

| 环境变量 | 指向的目录 | 目录内应包含 |
| --- | --- | --- |
| `MINGW_HOME` | MinGW 根目录 | `bin/gcc.exe` |
| `CMAKE_HOME` | CMake 根目录 | `bin/cmake.exe` |
| `NINJA_HOME` | Ninja 所在目录 | `ninja.exe` |

例如，本机可以在 PowerShell 中执行：

```powershell
[Environment]::SetEnvironmentVariable("MINGW_HOME", "C:\Program Files (x86)\Dev-Cpp\MinGW64", "User")
[Environment]::SetEnvironmentVariable("CMAKE_HOME", "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\CMake", "User")
[Environment]::SetEnvironmentVariable("NINJA_HOME", "D:\Program Files\Microsoft Visual Studio\2022\Community\Common7\IDE\CommonExtensions\Microsoft\CMake\Ninja", "User")
```

其他成员应将示例路径替换为自己电脑上的实际路径。设置后需要完全重启 VS Code。

## 编译运行

在 VS Code 中按 `Ctrl+Shift+B`，默认任务 `2048: Build and Run` 会依次完成 CMake 配置、编译并运行程序。

生成的程序位于：

```text
build/bin/2048_game.exe
```

用户注册数据保存在 `Data/user.txt`。
