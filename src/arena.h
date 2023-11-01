#pragma once

#include <stddef.h>

// https://nullprogram.com/blog/2023/09/27/
typedef struct {
    char *beg;
    char *end;
} arena;

arena new_arena(ptrdiff_t cap);

// // a: args which are returned to caller
// // scratch: temporary objects whose lifetime ends when the function returns
// // all allocations from a scratch arena are implicitly freed upon return
// retval *func(arena *a, arena scratch, int arg)
// {
//     new(&scratch, char_t, 32, 0);
//     return new(perm, retval, 1, 0);
// }
