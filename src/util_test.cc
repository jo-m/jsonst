#include <gtest/gtest.h>

extern "C" {
#include "util.h"
#undef new
#undef alignof
}

#define ARENA_SIZE 1000

TEST(UtilTest, NullTerm) {
    arena a = new_arena(ARENA_SIZE);
    {
        arena scratch = a;
        void *arena_fill = alloc(&scratch, 1, alignof(uint8_t), ARENA_SIZE, 0);
        memset(arena_fill, 'a', ARENA_SIZE);
    }

    char hello[] = "hello";
    EXPECT_EQ(strlen(hello), 5);
    EXPECT_EQ(sizeof(hello), 6);

    EXPECT_EQ(lengthof(hello), 5);
    EXPECT_EQ(countof(hello), 6);

    s8 s8hello = {hello, lengthof(hello)};
    EXPECT_EQ(s8hello.len, 5);
    EXPECT_EQ(s8hello.buf[s8hello.len], '\0');

    arena_free(a);
}
