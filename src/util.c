#include "util.h"

#include <stdlib.h>  // abort()
#include <string.h>  // memset()

// TODO: add flags
__attribute((malloc, alloc_size(2, 4), alloc_align(3))) void *alloc(arena *a, ptrdiff_t size,
                                                                    ptrdiff_t align,
                                                                    ptrdiff_t count,
                                                                    __attribute((unused))
                                                                    int32_t flags) {
    ptrdiff_t avail = a->end - a->beg;
    ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
    if (count > (avail - padding) / size) {
        abort();  // one possible out-of-memory policy
    }
    ptrdiff_t total = size * count;
    char *p = a->beg + padding;
    a->beg += padding + total;
    return memset(p, 0, total);
}

arena new_scratch(arena *a, const ptrdiff_t cap) {
    arena scratch = {0};
    scratch.beg = new (a, char, cap, 0);
    scratch.end = scratch.beg + cap;
    return scratch;
}

arena new_scratch_div(arena *a, const ptrdiff_t div) {
    const ptrdiff_t cap = (a->end - a->beg) / div;
    return new_scratch(a, cap);
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
    r.len = len;
    return r;
}

s8 s8span(char *beg, char *end) {
    s8 s = {0};
    s.buf = beg;
    s.len = end - beg;
    return s;
}

int32_t s8equal(s8 a, s8 b) {
    if (a.len != b.len) {
        return 0;
    }
    for (ptrdiff_t i = 0; i < a.len; i++) {
        if (a.buf[i] != b.buf[i]) {
            return 0;
        }
    }
    return 1;
}

ptrdiff_t s8cmp(s8 a, s8 b) {
    ptrdiff_t len = a.len < b.len ? a.len : b.len;
    for (ptrdiff_t i = 0; i < len; i++) {
        ptrdiff_t d = a.buf[i] - b.buf[i];
        if (d) {
            return d;
        }
    }
    return a.len - b.len;
}

s8 s8clone(arena *a, s8 s) {
    s8 c = new_s8(a, s.len);
    copy(c.buf, s.buf, s.len);
    return c;
}
