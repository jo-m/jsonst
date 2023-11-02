#include <gtest/gtest.h>

extern "C" {
#include "jsons.h"
#include "jsons_helpers.h"
}

void cb(const jsons_path *path, const jsons_value *value) {
    printf("jsons_event_cb(%p, %d, %s)\n", (void *)path, value->type,
           json_type_to_str(value->type));
}

void feed_doc(json_streamer j, const std::string doc) {
    for (const char &c : doc) {
        json_streamer_feed(j, c);
    }
}

void parse_doc(const char *doc) {
    arena a = new_arena(2000);
    EXPECT_NE(a.beg, nullptr);

    json_streamer j = new_json_streamer(a, cb);
    EXPECT_NE(j, nullptr);
    feed_doc(j, doc);

    json_streamer_terminate(j);
    arena_free(a);
}

TEST(JsonsTest, ListBasics) {
    parse_doc("[]");
    parse_doc("[ ]");
    parse_doc("[  ]");
    parse_doc(" [  ] ");
    parse_doc("[true]");
    parse_doc("[true,false]");
    parse_doc("[true,false,null]");
    parse_doc("[true,false,  null ] ");
    parse_doc(" [ true, false , null  ,[]  , 123 ] ");
    parse_doc(" [ true, false , null  ,[]  , 123] ");
    parse_doc(" [ true, false , null  ,[]  , 123, 123,true ] ");
}

TEST(JsonsTest, ListElem) { parse_doc("[ ]"); }

// TEST(JsonsTest, BasicAssertions) {
//     arena a = new_arena(1000);
//     EXPECT_NE(a.beg, nullptr);

//     json_streamer j = new_json_streamer(a, cb);
//     EXPECT_NE(j, nullptr);
//     feed_doc(j, "[,]");  // TODO: this should fail.

//     json_streamer_terminate(j);
//     arena_free(a);
// }

// TEST(JsonsTest, Numbers) {
//     arena a = new_arena(1000);
//     EXPECT_NE(a.beg, nullptr);

//     json_streamer j = new_json_streamer(a, cb);
//     EXPECT_NE(j, nullptr);
//     feed_doc(j, "[123, 45.32, 1e8, -2]");

//     json_streamer_terminate(j);
//     arena_free(a);
// }
