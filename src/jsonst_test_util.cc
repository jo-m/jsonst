#include "jsonst_test_util.h"

#include <gtest/gtest.h>

#include <string>

extern "C" {
#include "jsonst_util.h"
}

void null_cb(const jsonst_value __attribute((unused)) * v,
             const jsonst_path __attribute((unused)) * p) {}

jsonst_error parse_doc_to_err(const ptrdiff_t memsz, const std::string doc) {
    uint8_t *mem = new uint8_t[memsz];
    EXPECT_NE(mem, nullptr);

    jsonst_config conf = {0, 0, 0, nullptr};
    jsonst j = new_jsonst(mem, memsz, null_cb, conf);
    EXPECT_NE(j, nullptr);

    const auto ret = jsonst_feed_doc(j, doc.c_str(), doc.length());

    free(mem);

    return ret.err;
}

void expect_jsonst_success(const ptrdiff_t memsz, const std::string doc) {
    EXPECT_EQ(jsonst_success, parse_doc_to_err(memsz, doc));
}

static std::ostringstream o{};

void ost_cb(const jsonst_value *value, const jsonst_path *path) {
    // Path.
    o << '$';
    for (const jsonst_path *p = path; p != NULL; p = p->next) {
        switch (p->type) {
            case jsonst_arry_elm:
                o << '[' << p->props.arry_ix << ']';
                break;
            case jsonst_obj_key:
                o << '.' << std::string(p->props.obj_key.str, p->props.obj_key.str_len);
                break;
            default:
                break;
        }
    }
    o << '=';

    // Value.
    switch (value->type) {
        case jsonst_null:
        case jsonst_true:
        case jsonst_false:
            o << jsonst_type_to_str(value->type);
            break;
        case jsonst_num:
            o << '(' << jsonst_type_to_str(value->type) << ')' << value->val_num;
            break;
        case jsonst_str:
        case jsonst_obj_key:
            o << '(' << jsonst_type_to_str(value->type) << ')' << "'"
              << std::string(value->val_str.str, value->val_str.str_len) << "'";
            break;
        case jsonst_arry:
        case jsonst_arry_end:
        case jsonst_obj:
        case jsonst_obj_end:
            o << '(' << jsonst_type_to_str(value->type) << ')';
            break;
        default:
            o << jsonst_type_to_str(value->type) << '=' << value->type;
    }

    o << std::endl;
}

std::string parse_doc_to_txt(const ptrdiff_t memsz, const std::string doc,
                             const jsonst_config conf) {
    o.str("");
    o.clear();
    o << std::endl;

    uint8_t *mem = new uint8_t[memsz];
    EXPECT_NE(mem, nullptr);
    jsonst j = new_jsonst(mem, memsz, ost_cb, conf);
    EXPECT_NE(j, nullptr);

    const auto ret = jsonst_feed_doc(j, doc.c_str(), doc.length());
    free(mem);

    o << "ret=" << jsonst_error_to_str(ret.err) << std::endl;
    o << "parsed_bytes=" << ret.parsed_bytes << std::endl;

    return o.str();
}
