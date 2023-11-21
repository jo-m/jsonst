#include <gtest/gtest.h>

extern "C" {
#include "util.h"
#undef new
#undef alignof
}

TEST(UtilTest, NullTerm) {
    const ptrdiff_t memsz = 1024 * 1;
    uint8_t *mem = new uint8_t[memsz];
    EXPECT_NE(mem, nullptr);

    arena a = new_arena(mem, memsz);
    {
        arena scratch = a;
        void *arena_fill = alloc(&scratch, 1, alignof(int8_t), memsz);
        memset(arena_fill, 'a', memsz);
    }

    char hello[] = "hello";
    EXPECT_EQ(strlen(hello), 5);
    EXPECT_EQ(Sizeof(hello), 6);

    EXPECT_EQ(Lengthof(hello), 5);
    EXPECT_EQ(Countof(hello), 6);

    s8 s8hello = {hello, Lengthof(hello)};
    EXPECT_EQ(s8hello.len, 5);
    EXPECT_EQ(s8hello.buf[s8hello.len], '\0');

    free(mem);
}
