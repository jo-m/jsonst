#ifndef JSONST_TEST_UTIL_H
#define JSONST_TEST_UTIL_H

// jsonst_test_util.{h,cc}: Internal utilities for tests.

#include <string>

extern "C" {
#include "jsonst.h"
}

void null_cb(void *user_data, const jsonst_value *value, const jsonst_path *p);
jsonst_error parse_doc_to_err(const ptrdiff_t memsz, const std::string doc);
void expect_jsonst_success(const ptrdiff_t memsz, const std::string doc);
std::string parse_doc_to_txt(const ptrdiff_t memsz, const std::string doc,
                             const jsonst_config conf = {0, 0, 0});

#endif
