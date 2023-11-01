#include "arena.h"

#include <stdlib.h>

arena new_arena(const ptrdiff_t cap) {
    arena a = {0};
    a.beg = malloc(cap);
    a.end = a.beg ? a.beg + cap : 0;
    return a;
}
