cmake_minimum_required(VERSION 3.16)

project(
    2048Game
    VERSION 1.0.0
    DESCRIPTION "A modular 2048 game written in C"
    LANGUAGES C
)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

include_directories(Inc)

add_executable(2048_game
    Src/main.c
    Src/board.c
    Src/game.c
    Src/rank.c
    Src/save.c
    Src/storage.c
    Src/ui.c
    Src/user.c
    Src/utils.c
)

set_target_properties(2048_game PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

if(MSVC)
    target_compile_options(2048_game PRIVATE /W4 /utf-8)
else()
    target_compile_options(2048_game PRIVATE -Wall -Wextra)
endif()
