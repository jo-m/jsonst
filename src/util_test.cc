#include <gtest/gtest.h>

extern "C" {
#include "util.h"
#undef new
#undef alignof
}

TEST(UtilTest, NullTerm) {
    const ptrdiff_t memsz = 1024 * 1;
    char *mem = new char[memsz];
    EXPECT_NE(mem, nullptr);

    arena a = new_arena(mem, memsz);
    {
        arena scratch = a;
        void *arena_fill = alloc(&scratch, 1, alignof(int8_t), memsz, 0);
        memset(arena_fill, 'a', memsz);
    }

    char hello[] = "hello";
    EXPECT_EQ(strlen(hello), 5);
    EXPECT_EQ(sizeof(hello), 6);

    EXPECT_EQ(lengthof(hello), 5);
    EXPECT_EQ(countof(hello), 6);

    s8 s8hello = {hello, lengthof(hello)};
    EXPECT_EQ(s8hello.len, 5);
    EXPECT_EQ(s8hello.buf[s8hello.len], '\0');

    free(mem);
}
