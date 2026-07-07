#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "board.h"

FILE *storage_open_read(const char *file_path); // 打开文件以读取模式，如果文件不存在则返回NULL

FILE *storage_open_append(const char *file_path); // 打开文件以追加模式，如果文件不存在则创建新文件

FILE *storage_open_write(const char *file_path); // 打开文件以写入模式，如果文件不存在则创建新文件

bool storage_read_line(FILE *file, char *buffer, size_t buffer_size); // 从文件中读取一行文本，存储到buffer中，buffer_size为缓冲区大小，成功返回true，失败返回false

bool storage_write_user(FILE *file, const char *username, uint32_t hash1, uint32_t hash2); // 将用户名和双哈希值写入文件，成功返回true，失败返回false

bool storage_write_score(FILE *file, const char *username, int score); // 将用户名和加密分数写入文件，格式为"username\tencrypted_score\n"，成功返回true，失败返回false

bool storage_write_save(FILE *file,
                        const char *username,
                        const Board *board); // 写入一条游戏存档

bool storage_parse_save_line(const char *line,
                             char *username,
                             size_t username_size,
                             Board *board); // 解析一条游戏存档

bool storage_close(FILE *file); // 关闭文件，成功返回true，失败返回false

bool storage_remove_file(const char *file_path); // 删除文件，成功返回true，失败返回false

bool storage_replace_file(const char *source_path, const char *target_path); // 用source_path文件替换target_path文件，成功返回true，失败返回false
