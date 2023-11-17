#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

extern "C" {
#include "jsonst_cstd.h"
}

const char *testdata_fname = "src/testdata/test.json";

#define DEFAULT_MEMSZ (8 * 1024)

void null_cb(__attribute((unused)) void *user_data, __attribute((unused)) const jsonst_value *v,
             __attribute((unused)) const jsonst_path *p) {}

TEST(JsonstTest, ParseFstream) {
    FILE *f = fopen(testdata_fname, "r");
    ASSERT_NE(f, nullptr);

    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    ASSERT_NE(mem, nullptr);

    jsonst_config conf = {0, 0, 0, nullptr};
    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, nullptr, conf);
    ASSERT_NE(j, nullptr);

    const jsonst_feed_doc_ret ret = jsonst_feed_fstream(j, f);
    EXPECT_EQ(jsonst_success, ret.err);
    EXPECT_EQ(18, ret.parsed_bytes);

    free(mem);
}
