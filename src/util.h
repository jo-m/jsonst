#pragma once

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define sizeof(x) (ptrdiff_t)sizeof(x)
#define alignof(x) (ptrdiff_t) _Alignof(x)
#define countof(a) (sizeof(a) / sizeof(*(a)))
#define lengthof(s) (countof(s) - 1)

// Inspired by https://nullprogram.com/blog/2023/09/27/.
typedef struct {
    uint8_t *beg;
    uint8_t *end;
} arena;

arena new_arena(uint8_t *mem, const ptrdiff_t memsz);

// Returns NULL on OOM.
__attribute((malloc, alloc_size(2, 4), alloc_align(3))) void *alloc(arena *a, ptrdiff_t size,
                                                                    ptrdiff_t align,
                                                                    ptrdiff_t count, int32_t flags);

// May return arena.beg = NULL on OOM.
arena new_scratch(arena *a, ptrdiff_t cap);

// clang-format off
#define new(a, t, n, f)(t *) alloc(a, sizeof(t), _Alignof(t), n, f)
// clang-format on

#define s8(s) \
    (s8) { (char *)s, lengthof(s) }

// Inspired by https://nullprogram.com/blog/2023/10/08/, with a modification
// for interop with other C code:
// Guaranteed to always be null terminated, but the length field is excluding the
// terminating 0 byte.
typedef struct {
    char *buf;
    ptrdiff_t len;
} s8;

// May return s8.buf = NULL on OOM.
s8 new_s8(arena *a, ptrdiff_t len);
