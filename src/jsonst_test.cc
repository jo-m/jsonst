#include <gtest/gtest.h>

extern "C" {
#include "jsonst.h"
#include "jsonst_helpers.h"
}

void cb(const jsonst_value *value, const jsonst_path *path) {
    printf("jsonst_value_cb(%p, %d, %s)\n", (void *)path, value->type,
           jsonst_type_to_str(value->type));
}

void parse_doc(const std::string doc) {
    const ptrdiff_t memsz = 1024 * 8;
    uint8_t *mem = new uint8_t[memsz];
    EXPECT_NE(mem, nullptr);

    jsonst j = new_jsonst(mem, memsz, cb);
    EXPECT_NE(j, nullptr);

    for (ptrdiff_t i = 0; doc[i] != 0; i++) {
        jsonst_feed(j, doc[i]);
    }
    jsonst_feed(j, EOF);

    free(mem);
}

TEST(JsonstTest, Null) {
    parse_doc(" null");
    parse_doc("  null ");
}

TEST(JsonstTest, Bool) {
    parse_doc("true");
    parse_doc(" true ");
    parse_doc("false");
    parse_doc(" false ");
}

TEST(JsonstTest, Num) {
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

TEST(JsonstTest, Str) {
    parse_doc("\"\"");
    parse_doc("\"a\"");
    parse_doc("\"a\" ");
    parse_doc(" \"a\" ");
}

TEST(JsonstTest, StrUTF8) {
    parse_doc(" \"Aä\" ");
    parse_doc(" \"Aäö8ü-\" ");
    parse_doc("\"Aä\"");
    parse_doc("\"aa߿aa\"");
    parse_doc("\"aࠀaa\"");
    parse_doc("\"aa𐀀aa\"");
}

TEST(JsonstTest, StrEscape) {
    parse_doc("\"aa \\\\ \\\" \\r \\n aa\"");
    parse_doc("\" aa \\u1234 aa \"");
    parse_doc("\" \\u1a3C \\uFFFF \"");
    parse_doc("\" \\uD834\\uDD1E \"");  // UTF-16 surrogate pair.
}

TEST(JsonstTest, ObjKey) {
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

TEST(JsonstTest, ListSimple) {
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

TEST(JsonstTest, ListNested) {
    parse_doc("[[[],[[[[]]]], []]]");
    parse_doc("[[[ ],[[[ [] ],[], [],[[ []]], [] ]], []]]");
}

TEST(JsonstTest, ObjectSimple) {
    parse_doc("{}");
    parse_doc("{ }");
    parse_doc("{  }");
    parse_doc(" {   } ");
    parse_doc("{ \"ky\": null}  ");
    parse_doc("{ \"ky\": 123}  ");
    parse_doc("{ \"ky\": 123 }  ");
}

TEST(JsonstTest, ObjectNested) {
    parse_doc("{ \"1\":{\"1\":{}, \"2\":{\"1\":{}}}, \"2\":{}, \"3\":{} }");
}
