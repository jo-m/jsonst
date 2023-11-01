#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#include "arena.h"

#define sizeof(x) (ptrdiff_t)sizeof(x)
#define alignof(x) (ptrdiff_t) _Alignof(x)
#define countof(a) (sizeof(a) / sizeof(*(a)))
#define lengthof(s) (countof(s) - 1)

/*
// TODO: Maybe re-enable.

#define assert(c) \
    while (!(c)) __builtin_unreachable()
*/

__attribute((malloc, alloc_size(2, 4), alloc_align(3))) void *alloc(arena *a, ptrdiff_t size,
                                                                    ptrdiff_t align,
                                                                    ptrdiff_t count, int32_t flags);

arena new_scratch(arena *a, ptrdiff_t cap);

arena new_scratch_div(arena *a, ptrdiff_t div);

// clang-format off
#define new(a, t, n, f)(t *) alloc(a, sizeof(t), _Alignof(t), n, f)
// clang-format on

void copy(uint8_t *restrict dst, uint8_t *restrict src, ptrdiff_t len);

#define s8(s) \
    (s8) { (uint8_t *)s, lengthof(s) }

// https://nullprogram.com/blog/2023/10/08/
typedef struct {
    uint8_t *buf;
    ptrdiff_t len;
} s8;

s8 new_s8(arena *a, ptrdiff_t len);

s8 s8span(uint8_t *beg, uint8_t *end);

int32_t s8equal(s8 a, s8 b);

ptrdiff_t s8cmp(s8 a, s8 b);

s8 s8clone(arena *a, s8 s);
