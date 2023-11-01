#include <gtest/gtest.h>

extern "C" {
#include "jsons.h"
}

void cb(const jsons_path *path, const jsons_value *value) {
    printf("jsons_event_cb(%p, %d)\n", (void *)path, value->type);
}

TEST(JsonsTest, BasicAssertions) {
    arena a = new_arena(1000);
    json_streamer j = new_json_streamer(a, cb);
    json_streamer_feed(j, '1');

    EXPECT_EQ(7 * 6, 42);
}
