#include "util.h"

arena new_arena(uint8_t *mem, const ptrdiff_t memsz) {
    arena a = {0};
    a.beg = mem;
    a.end = a.beg ? a.beg + memsz : 0;
    return a;
}

__attribute((malloc, alloc_size(2, 4), alloc_align(3))) void *alloc(arena *a, ptrdiff_t size,
                                                                    ptrdiff_t align,
                                                                    ptrdiff_t count) {
    ptrdiff_t avail = a->end - a->beg;
    ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
    if (count > (avail - padding) / size) {
        return NULL;
    }
    ptrdiff_t total = size * count;
    uint8_t *p = a->beg + padding;
    a->beg += padding + total;

    // memset(p, 0, total)
    for (ptrdiff_t i = 0; i < total; i++) {
        p[i] = 0;
    }

#ifndef NDEBUG
    for (uint8_t *m = a->beg; m <= a->end; m++) {
        *m = '~';
    }
#endif

    return p;
}

void copy(char *restrict dst, char *restrict src, ptrdiff_t len) {
    for (ptrdiff_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }
}

s8 new_s8(arena *a, const ptrdiff_t len) {
    s8 s = {0};
    // +1 is for C 0 byte interop.
    s.buf = New (a, char, len + 1);
    if (s.buf != NULL) {
        s.len = len;
    }
    return s;
}
