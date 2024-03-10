// Test jsonst against the JSON Test Suite
// (https://github.com/nst/JSONTestSuite).

#include <gtest/gtest-spi.h>
#include <gtest/gtest.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

extern "C" {
#include "jsonst.h"
}
#include "jsonst_test_util.h"

namespace fs = std::filesystem;

const std::string test_parsing_files = "external/com_github_nst_json_test_suite/test_parsing";
const std::string test_transform_files = "external/com_github_nst_json_test_suite/test_transform";
const size_t memsz = 1024 * 8;

class FileTest : public testing::TestWithParam<fs::directory_entry> {
   public:
    void SetUp() override {
        mem = new uint8_t[memsz];
        EXPECT_NE(mem, nullptr);

        conf = {0, 0, 0};
        j = new_jsonst(mem, memsz, null_cb, nullptr, conf);
        EXPECT_NE(j, nullptr);
    }

    void TearDown() override { free(mem); }

    jsonst_feed_doc_ret feed_testfile(fs::path path) {
        jsonst_feed_doc_ret ret = {jsonst_success, 0};
        std::ifstream testfile(path);
        std::istreambuf_iterator<char> it{testfile}, end;
        for (; it != end; ++it) {
            ret.err = jsonst_feed(j, *it);
            if (ret.err != jsonst_success) {
                return ret;
            }
            ret.parsed_bytes++;
        }

        ret.err = jsonst_feed(j, JSONST_EOF);
        return ret;
    }

    uint8_t* mem;
    jsonst_config conf;
    jsonst j;
};

std::vector<fs::directory_entry> list_test_files(const std::string& path) {
    std::vector<fs::directory_entry> ret{};
    for (const auto& entry : fs::directory_iterator(path)) {
        ret.push_back(entry);
    }

    return ret;
}

std::string get_test_name(testing::TestParamInfo<fs::directory_entry> info) {
    auto ret = info.param.path().filename().string();
    for (char& c : ret) {
        if (!isalnum(c)) {
            c = '_';
        }
    }
    return ret + "_" + std::to_string(info.index);
}

TEST_P(FileTest, ParseFile) {
    const auto entry = GetParam();
    const auto fname = entry.path().filename().string();
    std::cout << fname << std::endl;
    const jsonst_feed_doc_ret ret = feed_testfile(entry.path());

    if (fname.rfind("n_", 0) == 0) {
        EXPECT_NE(jsonst_success, ret.err);
        return;
    }

    if (fname.rfind("y_", 0) == 0) {
        EXPECT_EQ(jsonst_success, ret.err);
        EXPECT_EQ(entry.file_size(), ret.parsed_bytes);
        return;
    }

    // For the rest, we are happy enough if we don't crash.
}

INSTANTIATE_TEST_SUITE_P(ParseFile_parsing_files, FileTest,
                         ::testing::ValuesIn(list_test_files(test_parsing_files)), get_test_name);

INSTANTIATE_TEST_SUITE_P(ParseFile_transform_files, FileTest,
                         ::testing::ValuesIn(list_test_files(test_transform_files)), get_test_name);
