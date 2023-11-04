#pragma once

#include <stddef.h>

// Inspired by https://nullprogram.com/blog/2023/09/27/.
typedef struct {
    // TODO: maybe make uint8_t
    char *beg;
    char *end;
} arena;

arena new_arena(ptrdiff_t cap);

void arena_free(arena a);
