#pragma once

#include <string>

extern "C" {
#include "jsonst.h"
}

void null_cb(const jsonst_value *value, const jsonst_path *p);
jsonst_error parse_doc_to_err(const ptrdiff_t memsz, const std::string doc);
void expect_jsonst_success(const ptrdiff_t memsz, const std::string doc);
std::string parse_doc_to_txt(const ptrdiff_t memsz, const std::string doc,
                             const jsonst_config conf = {0, 0, 0, nullptr});
