#include <gtest/gtest.h>

extern "C" {
#include "jsons.h"
#include "jsons_helpers.h"
}

void cb(const jsons_value *value, const jsons_path *path) {
    printf("jsons_event_cb(%p, %d, %s)\n", (void *)path, value->type,
           json_type_to_str(value->type));
}

void parse_doc(const std::string doc) {
    const ptrdiff_t memsz = 1024 * 8;
    char *mem = new char[memsz];
    EXPECT_NE(mem, nullptr);

    json_streamer j = new_json_streamer(mem, memsz, cb);
    EXPECT_NE(j, nullptr);

    for (ptrdiff_t i = 0; doc[i] != 0; i++) {
        json_streamer_feed(j, doc[i]);
    }
    json_streamer_feed(j, EOF);

    free(mem);
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

TEST(JsonsTest, ObjKey) {
    parse_doc("{ \"Aä\" :123 }");
    parse_doc("{ \"Aäö8ü-\" :123 }");
    parse_doc("{ \"Aä\" :123 }");
    parse_doc("{ \"aa߿aa\" :123 }");
    parse_doc("{ \"aࠀaa\" :123 }");
    parse_doc("{ \"aa \\\\ \\\" \\r \\n aa\":123 }");
    parse_doc("{ \" aa \\u1234 aa \":123 }");
    parse_doc("{ \" \\u1a3C \\uFFFF \":123 }");
    parse_doc("{ \" \\uD834\\uDD1E \":123 }");
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
