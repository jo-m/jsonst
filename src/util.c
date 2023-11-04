#include "util.h"

#include <assert.h>

arena new_arena(uint8_t *mem, const ptrdiff_t memsz) {
    assert(mem != NULL);
    assert(memsz != 0);

    arena a = {0};
    a.beg = mem;
    a.end = a.beg ? a.beg + memsz : 0;
    return a;
}

__attribute((malloc, alloc_size(2, 4), alloc_align(3))) void *alloc(arena *a, ptrdiff_t size,
                                                                    ptrdiff_t align,
                                                                    ptrdiff_t count,
                                                                    __attribute((unused))
                                                                    int32_t flags) {
    ptrdiff_t avail = a->end - a->beg;
    ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
    if (count > (avail - padding) / size) {
        return NULL;
    }
    ptrdiff_t total = size * count;
    uint8_t *p = a->beg + padding;
    a->beg += padding + total;

    // memset(p, 0, total)
    for (int i = 0; i < total; i++) {
        p[i] = 0;
    }
    return p;
}

arena new_scratch(arena *a, const ptrdiff_t cap) {
    arena scratch = {0};
    scratch.beg = new (a, uint8_t, cap, 0);
    if (scratch.beg != NULL) {
        scratch.end = scratch.beg + cap;
    }
    return scratch;
}

void copy(char *restrict dst, char *restrict src, ptrdiff_t len) {
    for (ptrdiff_t i = 0; i < len; i++) {
        dst[i] = src[i];
    }
}

s8 new_s8(arena *a, const ptrdiff_t len) {
    s8 r = {0};
    // +1 is for C 0 byte interop.
    r.buf = new (a, char, len + 1, 0);
    if (r.buf != NULL) {
        r.len = len;
    }
    return r;
}
