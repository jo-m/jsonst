#pragma once

#include <stddef.h>

// https://nullprogram.com/blog/2023/09/27/
typedef struct {
    // TODO: maybe make uint8_t
    char *beg;
    char *end;
} arena;

// TODO: hide this completely, and let user pass in a memory buf + length instead.
arena new_arena(ptrdiff_t cap);

void arena_free(arena a);

// TODO: improve or remove
// // a: args which are returned to caller
// // scratch: temporary objects whose lifetime ends when the function returns
// // all allocations from a scratch arena are implicitly freed upon return
// retval *func(arena *a, arena scratch, int arg)
// {
//     new(&scratch, char_t, 32, 0);
//     return new(perm, retval, 1, 0);
// }
