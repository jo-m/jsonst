#include <gtest/gtest.h>

extern "C" {
#include "jsonst.h"
#include "jsonst_helpers.h"
}

void cb(const jsonst_value *value, const jsonst_path __attribute((unused)) * p_first,
        const jsonst_path __attribute((unused)) * p_last) {
    printf("jsonst_value_cb(%d, %s)\n", value->type, jsonst_type_to_str(value->type));
}

#define DEFAULT_MEMSZ (8 * 1024)

void parse_doc(const ptrdiff_t memsz, const std::string doc) {
    uint8_t *mem = new uint8_t[memsz];
    EXPECT_NE(mem, nullptr);

    jsonst j = new_jsonst(mem, memsz, cb);
    EXPECT_NE(j, nullptr);

    const jsonst_error ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    EXPECT_EQ(ret, jsonst_success);

    free(mem);
}

TEST(JsonstTest, ErrorLivecycle) {
    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    EXPECT_NE(mem, nullptr);

    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, cb);
    EXPECT_EQ(jsonst_success, jsonst_feed(j, '{'));
    EXPECT_EQ(jsonst_success, jsonst_feed(j, '}'));
    EXPECT_EQ(jsonst_success, jsonst_feed(j, EOF));
    EXPECT_EQ(jsonst_success, jsonst_feed(j, ' '));
    EXPECT_EQ(jsonst_err_end_of_doc, jsonst_feed(j, '{'));
    EXPECT_EQ(jsonst_err_previous_error, jsonst_feed(j, ' '));
    EXPECT_EQ(jsonst_err_previous_error, jsonst_feed(j, ' '));
}

TEST(JsonstTest, Null) {
    parse_doc(DEFAULT_MEMSZ, " null");
    parse_doc(DEFAULT_MEMSZ, "  null ");
}

TEST(JsonstTest, Bool) {
    parse_doc(DEFAULT_MEMSZ, "true");
    parse_doc(DEFAULT_MEMSZ, " true ");
    parse_doc(DEFAULT_MEMSZ, "false");
    parse_doc(DEFAULT_MEMSZ, " false ");
}

TEST(JsonstTest, Num) {
    parse_doc(DEFAULT_MEMSZ, "0");
    parse_doc(DEFAULT_MEMSZ, "0 ");
    parse_doc(DEFAULT_MEMSZ, "1");
    parse_doc(DEFAULT_MEMSZ, " 1");
    parse_doc(DEFAULT_MEMSZ, " 1 ");
    parse_doc(DEFAULT_MEMSZ, "1 ");
    parse_doc(DEFAULT_MEMSZ, "123");
    parse_doc(DEFAULT_MEMSZ, "123 ");
    parse_doc(DEFAULT_MEMSZ, " 123");
    parse_doc(DEFAULT_MEMSZ, " 123 ");
    parse_doc(DEFAULT_MEMSZ, " 0.0 ");
    parse_doc(DEFAULT_MEMSZ, " 0.1 ");
    parse_doc(DEFAULT_MEMSZ, " 0.1234 ");
    parse_doc(DEFAULT_MEMSZ, " 1.1234 ");
    parse_doc(DEFAULT_MEMSZ, " 0.1234 ");
}

TEST(JsonstTest, Str) {
    parse_doc(DEFAULT_MEMSZ, "\"\"");
    parse_doc(DEFAULT_MEMSZ, "\"a\"");
    parse_doc(DEFAULT_MEMSZ, "\"a\" ");
    parse_doc(DEFAULT_MEMSZ, " \"a\" ");
}

TEST(JsonstTest, StrUTF8) {
    parse_doc(DEFAULT_MEMSZ, " \"Aä\" ");
    parse_doc(DEFAULT_MEMSZ, " \"Aäö8ü-\" ");
    parse_doc(DEFAULT_MEMSZ, "\"Aä\"");
    parse_doc(DEFAULT_MEMSZ, "\"aa߿aa\"");
    parse_doc(DEFAULT_MEMSZ, "\"aࠀaa\"");
    parse_doc(DEFAULT_MEMSZ, "\"aa𐀀aa\"");
}

TEST(JsonstTest, StrEscape) {
    parse_doc(DEFAULT_MEMSZ, "\"aa \\\\ \\\" \\r \\n aa\"");
    parse_doc(DEFAULT_MEMSZ, "\" aa \\u1234 aa \"");
    parse_doc(DEFAULT_MEMSZ, "\" \\u1a3C \\uFFFF \"");
    parse_doc(DEFAULT_MEMSZ, "\" \\uD834\\uDD1E \"");  // UTF-16 surrogate pair.
}

TEST(JsonstTest, ObjKey) {
    parse_doc(DEFAULT_MEMSZ, "{ \"Aä\" :123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \"Aäö8ü-\" :123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \"Aä\" :123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \"aa߿aa\" :123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \"aࠀaa\" :123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \"aa \\\\ \\\" \\r \\n aa\":123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \" aa \\u1234 aa \":123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \" \\u1a3C \\uFFFF \":123 }");
    parse_doc(DEFAULT_MEMSZ, "{ \" \\uD834\\uDD1E \":123 }");
}

TEST(JsonstTest, ListSimple) {
    parse_doc(DEFAULT_MEMSZ, "[]");
    parse_doc(DEFAULT_MEMSZ, "[ ]");
    parse_doc(DEFAULT_MEMSZ, "[  ]");
    parse_doc(DEFAULT_MEMSZ, " [  ] ");
    parse_doc(DEFAULT_MEMSZ, "[true]");
    parse_doc(DEFAULT_MEMSZ, "[true,false]");
    parse_doc(DEFAULT_MEMSZ, "[true,false,null]");
    parse_doc(DEFAULT_MEMSZ, "[true,false,  null ] ");
    parse_doc(DEFAULT_MEMSZ, " [ true, false , null  ,[]  , 123 ] ");
    parse_doc(DEFAULT_MEMSZ, " [ true, false , null  ,[]  , 123] ");
    parse_doc(DEFAULT_MEMSZ, " [ true, false , null  ,[]  , 123, 123,true ] ");
}

TEST(JsonstTest, ListNested) {
    parse_doc(DEFAULT_MEMSZ, "[[[],[[[[]]]], []]]");
    parse_doc(DEFAULT_MEMSZ, "[[[ ],[[[ [] ],[], [],[[ []]], [] ]], []]]");
}

TEST(JsonstTest, ObjectSimple) {
    parse_doc(DEFAULT_MEMSZ, "{}");
    parse_doc(DEFAULT_MEMSZ, "{ }");
    parse_doc(DEFAULT_MEMSZ, "{  }");
    parse_doc(DEFAULT_MEMSZ, " {   } ");
    parse_doc(DEFAULT_MEMSZ, "{ \"ky\": null}  ");
    parse_doc(DEFAULT_MEMSZ, "{ \"ky\": 123}  ");
    parse_doc(DEFAULT_MEMSZ, "{ \"ky\": 123 }  ");
}

TEST(JsonstTest, ObjectNested) {
    parse_doc(DEFAULT_MEMSZ, "{ \"1\":{\"1\":{}, \"2\":{\"1\":{}}}, \"2\":{}, \"3\":{} }");
}
