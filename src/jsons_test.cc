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

    json_streamer_feed(j, 0);
    arena_free(a);
}

TEST(JsonsTest, Null) {
    parse_doc(" null");
    parse_doc("  null ");
}

TEST(JsonsTest, Bool) {
    parse_doc("true");
    parse_doc(" true ");
    parse_doc("false");
    parse_doc(" false ");
}

TEST(JsonsTest, Num) {
    parse_doc("0");
    parse_doc("0 ");
    parse_doc("1");
    parse_doc(" 1");
    parse_doc(" 1 ");
    parse_doc("1 ");
    parse_doc("123");
    parse_doc("123 ");
    parse_doc(" 123");
    parse_doc(" 123 ");
    parse_doc(" 0.0 ");
    parse_doc(" 0.1 ");
    parse_doc(" 0.1234 ");
    parse_doc(" 1.1234 ");
    parse_doc(" 0.1234 ");
}

TEST(JsonsTest, Str) {
    parse_doc("\"\"");
    parse_doc("\"a\"");
    parse_doc("\"a\" ");
    parse_doc(" \"a\" ");
}

TEST(JsonsTest, StrUTF8) {
    parse_doc(" \"Aä\" ");
    parse_doc(" \"Aäö8ü-\" ");
    parse_doc("\"Aä\"");
    parse_doc("\"aa߿aa\"");
    parse_doc("\"aࠀaa\"");
    parse_doc("\"aa𐀀aa\"");
}

TEST(JsonsTest, StrEscape) {
    parse_doc("\"aa \\\\ \\\" \\r \\n aa\"");
    parse_doc("\" aa \\u1234 aa \"");
    parse_doc("\" \\u1a3C \\uFFFF \"");
    parse_doc("\" \\uD834\\uDD1E \"");  // UTF-16 surrogate pair.
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
