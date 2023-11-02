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

    {
        arena scratch = a;
        s8 cloned = s8clone(&scratch, s8hello);
        EXPECT_EQ(cloned.len, s8hello.len);
        EXPECT_EQ(s8cmp(s8hello, cloned), 0);
        EXPECT_TRUE(s8equal(s8hello, cloned));

        EXPECT_EQ(cloned.buf[cloned.len], '\0');
        EXPECT_EQ(strlen(cloned.buf), 5);
    }

    arena_free(a);
}
