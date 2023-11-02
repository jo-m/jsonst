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
    arena a = new_arena(3000);
    EXPECT_NE(a.beg, nullptr);

    json_streamer j = new_json_streamer(a, cb);
    EXPECT_NE(j, nullptr);
    feed_doc(j, doc);

    json_streamer_terminate(j);
    arena_free(a);
}

TEST(JsonsTest, ListSimple) {
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

TEST(JsonsTest, ListNested) {
    parse_doc("[[[],[[[[]]]], []]]");
    parse_doc("[[[ ],[[[ [] ],[], [],[[ []]], [] ]], []]]");
}

TEST(JsonsTest, ObjectSimple) {
    parse_doc("{}");
    parse_doc("{ }");
    parse_doc("{  }");
    parse_doc(" {   } ");
    parse_doc("{ \"ky\": null}  ");
    parse_doc("{ \"ky\": 123}  ");
    parse_doc("{ \"ky\": 123 }  ");
}

TEST(JsonsTest, ObjectNested) {
    parse_doc("{ \"1\":{\"1\":{}, \"2\":{\"1\":{}}}, \"2\":{}, \"3\":{} }");
}
