#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

#include <cstring>

extern "C" {
#include "jsonst.h"
}
#include "jsonst_test_util.h"

#define DEFAULT_MEMSZ (8 * 1024)

TEST(JsonstTest, ErrorLivecycle) {
    uint8_t *mem = new uint8_t[DEFAULT_MEMSZ];
    ASSERT_NE(mem, nullptr);

    jsonst_config conf = {0, 0, 0};
    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, null_cb, nullptr, conf);
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
    ASSERT_DEATH(new_jsonst(mem, DEFAULT_MEMSZ, nullptr, nullptr, conf), "cb != NULL");
}

TEST(JsonstTest, ConfigAllocStr) {
    jsonst_config conf = {2, 0, 0};

    EXPECT_EQ(R"(
$=(jsonst_str)'12'
ret=jsonst_success
parsed_bytes=4
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"("12")", conf));

    EXPECT_EQ(R"(
ret=jsonst_err_str_buffer_full
parsed_bytes=3
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"("123")", conf));
}

TEST(JsonstTest, ConfigAllocObjKey) {
    jsonst_config conf = {0, 3, 0};

    EXPECT_EQ(R"(
$=(jsonst_obj)
$.123=(jsonst_obj_key)'123'
$.123=jsonst_true
$=(jsonst_obj_end)
ret=jsonst_success
parsed_bytes=12
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"({"123":true})", conf));

    EXPECT_EQ(R"(
$=(jsonst_obj)
ret=jsonst_err_str_buffer_full
parsed_bytes=5
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"({"1234":true})", conf));
}

TEST(JsonstTest, ConfigAllocNum) {
    jsonst_config conf = {0, 0, 4};

    EXPECT_EQ(R"(
$=(jsonst_num)1234
ret=jsonst_success
parsed_bytes=4
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"(1234)", conf));

    EXPECT_EQ(R"(
ret=jsonst_err_str_buffer_full
parsed_bytes=4
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"(12345)", conf));
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
parsed_bytes=26
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"({"k": [123,true,"strval"]})"));
}

TEST(JsonstTest, Edgecases) {
    EXPECT_EQ(R"(
$=(jsonst_obj)
$=(jsonst_obj_end)
ret=jsonst_success
parsed_bytes=2
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"({})"));

    EXPECT_EQ(R"(
$=(jsonst_obj)
$.id=(jsonst_obj_key)'id'
$.id=(jsonst_num)0
ret=jsonst_err_expected_new_key
parsed_bytes=8
)",
              parse_doc_to_txt(DEFAULT_MEMSZ, R"({"id":0,})"));

    EXPECT_NE(jsonst_success, parse_doc_to_err(DEFAULT_MEMSZ, R"({"id":0,})"));
    EXPECT_NE(jsonst_success, parse_doc_to_err(DEFAULT_MEMSZ, R"({"id":true,})"));

    EXPECT_NE(jsonst_success, parse_doc_to_err(DEFAULT_MEMSZ, ""));

    EXPECT_NE(jsonst_success, parse_doc_to_err(DEFAULT_MEMSZ, " "));

    EXPECT_NE(jsonst_success, parse_doc_to_err(DEFAULT_MEMSZ, R"({"a": true} "x")"));

    EXPECT_NE(jsonst_success, parse_doc_to_err(DEFAULT_MEMSZ, R"([][])"));
}
