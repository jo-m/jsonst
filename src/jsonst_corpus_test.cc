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
const size_t memsz = 1024 * 8;

class FileParseTest : public testing::TestWithParam<fs::directory_entry> {
   public:
    void SetUp() override {
        mem = new uint8_t[memsz];
        EXPECT_NE(mem, nullptr);

        conf = {0, 0, 0, nullptr};
        j = new_jsonst(mem, memsz, null_cb, conf);
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

TEST_P(FileParseTest, ParamTest) {
    const auto entry = GetParam();
    std::cout << entry.path().filename().string() << std::endl;
    const jsonst_feed_doc_ret ret = feed_testfile(entry.path());

    switch (entry.path().filename().string()[0]) {
        case 'y':
            EXPECT_EQ(jsonst_success, ret.err);
            EXPECT_EQ(entry.file_size(), ret.parsed_bytes);
            break;
        case 'n':
            EXPECT_NE(jsonst_success, ret.err);
            break;
    }
}

std::vector<fs::directory_entry> list_test_files() {
    std::vector<fs::directory_entry> ret{};
    for (const auto& entry : fs::directory_iterator(test_parsing_files)) {
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

INSTANTIATE_TEST_CASE_P(FileParseTest_instance, FileParseTest,
                        ::testing::ValuesIn(list_test_files()), get_test_name);
