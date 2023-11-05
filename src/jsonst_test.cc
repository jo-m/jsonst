#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

extern "C" {
#include "jsonst.h"
}
#include "jsonst_test_util.h"

#define DEFAULT_MEMSZ (8 * 1024)

TEST(JsonstTest, ErrorLivecycle) {
    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    ASSERT_NE(mem, nullptr);

    jsonst_config conf = {0, 0, 0};
    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, conf);
    EXPECT_EQ(jsonst_success, jsonst_feed(j, '{'));
    EXPECT_EQ(jsonst_success, jsonst_feed(j, '}'));
    EXPECT_EQ(jsonst_success, jsonst_feed(j, JSONST_EOF));
    EXPECT_EQ(jsonst_success, jsonst_feed(j, ' '));
    EXPECT_EQ(jsonst_err_end_of_doc, jsonst_feed(j, '{'));
    EXPECT_EQ(jsonst_err_previous_error, jsonst_feed(j, ' '));
    EXPECT_EQ(jsonst_err_previous_error, jsonst_feed(j, ' '));
}

TEST(JsonstTest, NoCb) {
    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    EXPECT_NE(mem, nullptr);

    jsonst_config conf = {0, 0, 0};
    ASSERT_DEATH(new_jsonst(mem, DEFAULT_MEMSZ, NULL, conf), "cb != NULL");
}

TEST(JsonstTest, ConfigAllocStr) {
    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    ASSERT_NE(mem, nullptr);

    jsonst_config conf = {2, 0, 0};
    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, conf);
    std::string doc = R"("12")";
    auto ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    EXPECT_EQ(jsonst_success, ret.err);
    EXPECT_EQ(doc.length(), ret.parsed_chars);

    j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, conf);
    doc = R"("123")";
    ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    EXPECT_EQ(jsonst_err_str_buffer_full, ret.err);
}

TEST(JsonstTest, ConfigAllocObjKey) {
    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    ASSERT_NE(mem, nullptr);

    jsonst_config conf = {0, 3, 0};
    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, conf);
    std::string doc = R"({"123":true})";
    auto ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    EXPECT_EQ(jsonst_success, ret.err);
    EXPECT_EQ(doc.length(), ret.parsed_chars);

    j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, conf);
    doc = R"({"1234":true})";
    ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    EXPECT_EQ(jsonst_err_str_buffer_full, ret.err);
}

TEST(JsonstTest, ConfigAllocNum) {
    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    ASSERT_NE(mem, nullptr);

    jsonst_config conf = {0, 0, 4};
    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, conf);
    std::string doc = R"(1234)";
    auto ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    EXPECT_EQ(jsonst_success, ret.err);
    EXPECT_EQ(doc.length(), ret.parsed_chars);

    j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, conf);
    doc = R"(12345)";
    ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    EXPECT_EQ(jsonst_err_str_buffer_full, ret.err);
}

TEST(JsonstTest, Null) {
    expect_jsonst_success(DEFAULT_MEMSZ, " null");
    expect_jsonst_success(DEFAULT_MEMSZ, "  null ");
}

TEST(JsonstTest, Bool) {
    expect_jsonst_success(DEFAULT_MEMSZ, "true");
    expect_jsonst_success(DEFAULT_MEMSZ, " true ");
    expect_jsonst_success(DEFAULT_MEMSZ, "false");
    expect_jsonst_success(DEFAULT_MEMSZ, " false ");
}

TEST(JsonstTest, Num) {
    expect_jsonst_success(DEFAULT_MEMSZ, "0");
    expect_jsonst_success(DEFAULT_MEMSZ, "0 ");
    expect_jsonst_success(DEFAULT_MEMSZ, "1");
    expect_jsonst_success(DEFAULT_MEMSZ, " 1");
    expect_jsonst_success(DEFAULT_MEMSZ, " 1 ");
    expect_jsonst_success(DEFAULT_MEMSZ, "1 ");
    expect_jsonst_success(DEFAULT_MEMSZ, "123");
    expect_jsonst_success(DEFAULT_MEMSZ, "123 ");
    expect_jsonst_success(DEFAULT_MEMSZ, " 123");
    expect_jsonst_success(DEFAULT_MEMSZ, " 123 ");
    expect_jsonst_success(DEFAULT_MEMSZ, " 0.0 ");
    expect_jsonst_success(DEFAULT_MEMSZ, " 0.1 ");
    expect_jsonst_success(DEFAULT_MEMSZ, " 0.1234 ");
    expect_jsonst_success(DEFAULT_MEMSZ, " 1.1234 ");
    expect_jsonst_success(DEFAULT_MEMSZ, " 0.1234 ");
}

TEST(JsonstTest, Str) {
    expect_jsonst_success(DEFAULT_MEMSZ, R"("")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"("a")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"("a" )");
    expect_jsonst_success(DEFAULT_MEMSZ, R"( "a" )");
}

TEST(JsonstTest, StrUTF8) {
    expect_jsonst_success(DEFAULT_MEMSZ, R"( "Aä" )");
    expect_jsonst_success(DEFAULT_MEMSZ, R"( "Aäö8ü-" )");
    expect_jsonst_success(DEFAULT_MEMSZ, R"("Aä")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"("aa߿aa")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"("aࠀaa")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"("aa𐀀aa")");
}

TEST(JsonstTest, StrEscape) {
    expect_jsonst_success(DEFAULT_MEMSZ, R"("aa \\ \" \r \n aa")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"(" aa \u1234 aa ")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"(" \u1a3C \uFFFF ")");
    expect_jsonst_success(DEFAULT_MEMSZ, R"(" \uD834\uDD1E ")");  // UTF-16 surrogate pair.
}

TEST(JsonstTest, ObjKey) {
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "Aä" :123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "Aäö8ü-" :123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "Aä" :123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "aa߿aa" :123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "aࠀaa" :123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "aa \\ \" \r \n aa":123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ " aa \u1234 aa ":123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ " \u1a3C \uFFFF ":123 })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ " \uD834\uDD1E ":123 })");
}

TEST(JsonstTest, ListSimple) {
    expect_jsonst_success(DEFAULT_MEMSZ, "[]");
    expect_jsonst_success(DEFAULT_MEMSZ, "[ ]");
    expect_jsonst_success(DEFAULT_MEMSZ, "[  ]");
    expect_jsonst_success(DEFAULT_MEMSZ, " [  ] ");
    expect_jsonst_success(DEFAULT_MEMSZ, "[true]");
    expect_jsonst_success(DEFAULT_MEMSZ, "[true,false]");
    expect_jsonst_success(DEFAULT_MEMSZ, "[true,false,null]");
    expect_jsonst_success(DEFAULT_MEMSZ, "[true,false,  null ] ");
    expect_jsonst_success(DEFAULT_MEMSZ, " [ true, false , null  ,[]  , 123 ] ");
    expect_jsonst_success(DEFAULT_MEMSZ, " [ true, false , null  ,[]  , 123] ");
    expect_jsonst_success(DEFAULT_MEMSZ, " [ true, false , null  ,[]  , 123, 123,true ] ");
}

TEST(JsonstTest, ListNested) {
    expect_jsonst_success(DEFAULT_MEMSZ, "[[[],[[[[]]]], []]]");
    expect_jsonst_success(DEFAULT_MEMSZ, "[[[ ],[[[ [] ],[], [],[[ []]], [] ]], []]]");
}

TEST(JsonstTest, ObjectSimple) {
    expect_jsonst_success(DEFAULT_MEMSZ, R"({})");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({  })");
    expect_jsonst_success(DEFAULT_MEMSZ, R"( {   } )");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "ky": null}  )");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "ky": 123}  )");
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "ky": 123 }  )");
}

TEST(JsonstTest, ObjectNested) {
    expect_jsonst_success(DEFAULT_MEMSZ, R"({ "1":{"1":{}, "2":{"1":{}}}, "2":{}, "3":{} })");
}

TEST(JsonstTest, Txt) {
    EXPECT_EQ(R"(
$=(jsonst_obj)
$.k=(jsonst_obj_key)'k'
$.k=(jsonst_arry)
$.k[0]=(jsonst_num)123
$.k[1]=jsonst_true
$.k[2]=(jsonst_str)'strval'
$.k=(jsonst_arry_end)
$=(jsonst_obj_end)
ret=jsonst_success
parsed_chars=26
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"({"k": [123,true,"strval"]})"));
}
