cmake_minimum_required(VERSION 3.16)

project(
    2048Game
    VERSION 1.0.0
    DESCRIPTION "A modular 2048 game written in C"
    LANGUAGES C
)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(GAME_CORE_SOURCES
    Src/board.c
    Src/game.c
    Src/rank.c
    Src/storage.c
    Src/ui.c
    Src/user.c
    Src/utils.c
)

set(GAME_PUBLIC_HEADERS
    Inc/board.h
    Inc/game.h
    Inc/rank.h
    Inc/storage.h
    Inc/ui.h
    Inc/user.h
    Inc/utils.h
)

# 核心业务模块，便于后续添加单元测试或其他前端。
add_library(game_core STATIC
    ${GAME_CORE_SOURCES}
    ${GAME_PUBLIC_HEADERS}
)

target_compile_features(game_core PUBLIC c_std_11)
target_include_directories(game_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/Inc>
        $<INSTALL_INTERFACE:include>
)

add_executable(2048_game Src/main.c)
target_link_libraries(2048_game PRIVATE game_core)

function(enable_project_warnings target_name)
    if(MSVC)
        target_compile_options(${target_name} PRIVATE /W4 /permissive- /utf-8)
    else()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra)
    endif()
endfunction()

enable_project_warnings(game_core)
enable_project_warnings(2048_game)

# 单配置与多配置生成器均输出到 build/bin，方便 VS Code 任务直接运行。
set_target_properties(2048_game PROPERTIES
    OUTPUT_NAME "2048_game"
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

foreach(config Debug Release RelWithDebInfo MinSizeRel)
    string(TOUPPER "${config}" config_upper)
    set_target_properties(2048_game PROPERTIES
        "RUNTIME_OUTPUT_DIRECTORY_${config_upper}" "${CMAKE_BINARY_DIR}/bin"
    )
endforeach()

# 构建完成后将运行所需的数据文件复制到可执行文件旁边。
add_custom_command(
    TARGET 2048_game
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Data"
            "$<TARGET_FILE_DIR:2048_game>/Data"
    COMMENT "Copying game data files"
)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Src" PREFIX "Source Files" FILES ${GAME_CORE_SOURCES})
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/Inc" PREFIX "Header Files" FILES ${GAME_PUBLIC_HEADERS})

install(TARGETS 2048_game RUNTIME DESTINATION bin)
install(DIRECTORY Data/ DESTINATION bin/Data)
