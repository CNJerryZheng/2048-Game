#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

FILE *storage_open_read(const char *file_path);

FILE *storage_open_append(const char *file_path);

bool storage_read_line(FILE *file, char *buffer, size_t buffer_size);

bool storage_write_user(FILE *file, const char *username, const char *password);

bool storage_close(FILE *file);
