#include "arena.h"

#include <stdlib.h>

arena new_arena(const ptrdiff_t cap) {
    arena a = {0};
    a.beg = malloc(cap);  // TODO: pass in malloc.
    a.end = a.beg ? a.beg + cap : 0;
    return a;
}

void arena_free(arena a) {
    free(a.beg);  // TODO: pass in free.
}
