cmake_minimum_required(VERSION 3.16)

project(2048GameQt VERSION 1.0 LANGUAGES C CXX)

configure_file(
    Inc/generated_config.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/generated/generated_config.h
    @ONLY
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets)
qt_standard_project_setup()

add_library(game_core STATIC
    Src/auth.c
    Src/board.c
    Src/game_mode.c
    Src/history.c
    Src/rank.c
    Src/storage.c
    Src/utils.c
)
target_include_directories(game_core PUBLIC
    Inc
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
if(MSVC)
    target_compile_definitions(game_core PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_options(game_core PRIVATE /W4 /utf-8 /wd4146)
else()
    target_compile_options(game_core PRIVATE -Wall -Wextra)
endif()

qt_add_executable(2048_game_qt WIN32
    Src/main.cpp
    Src/mainwindow.cpp
    Inc/mainwindow.h
)
target_link_libraries(2048_game_qt PRIVATE game_core Qt6::Widgets)
target_include_directories(2048_game_qt PRIVATE Inc)

set_target_properties(2048_game_qt PROPERTIES
    WIN32_EXECUTABLE TRUE
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

include(GNUInstallDirs)
install(TARGETS 2048_game_qt
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
