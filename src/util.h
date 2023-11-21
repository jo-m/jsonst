#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define Sizeof(x) (ptrdiff_t)sizeof(x)
#define Alignof(x) (ptrdiff_t) _Alignof(x)
#define Countof(a) (Sizeof(a) / Sizeof(*(a)))
#define Lengthof(s) (Countof(s) - 1)

// Inspired by https://nullprogram.com/blog/2023/09/27/.
typedef struct {
    uint8_t *beg;
    uint8_t *end;
} arena;

arena new_arena(uint8_t *mem, const ptrdiff_t memsz);

// Returns NULL on OOM.
__attribute((malloc, alloc_size(2), alloc_align(3))) void *alloc(arena *a, ptrdiff_t size,
                                                                 ptrdiff_t align, ptrdiff_t count)
    __attribute((warn_unused_result));

// clang-format off
#define New(a, t, n)(t *) alloc(a, Sizeof(t), _Alignof(t), n)
// clang-format on

#define s8(s) \
    (s8) { (char *)s, lengthof(s) }

// Inspired by https://nullprogram.com/blog/2023/10/08/, modified to always be null byte terminated.
// The length field is excluding the 0 byte.
typedef struct {
    char *buf;
    ptrdiff_t len;
} s8;

// May return s8.buf = NULL on OOM.
s8 new_s8(arena *a, ptrdiff_t len) __attribute((warn_unused_result));
