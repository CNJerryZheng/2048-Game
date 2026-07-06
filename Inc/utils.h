#pragma once

#include <stdbool.h>
#include <stddef.h>

bool utils_string_equal(const char *a, const char *b);

bool utils_read_line(const char *prompt, char *buffer, size_t size);

bool utils_read_choice(char *choice);

void utils_copy_string(char *target, const char *source, size_t target_size);
