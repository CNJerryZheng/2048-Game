#pragma once

#include <stdbool.h>

#include "board.h"

bool save_exists(const char *saves_file, const char *username);
bool save_load(const char *saves_file, const char *username, Board *board);
bool save_persist(const char *saves_file, const char *username, const Board *board);
bool save_delete(const char *saves_file, const char *username);
