#include "jsonst_util.h"

const char *jsonst_type_to_str(jsonst_type type) {
    switch (type) {
        case jsonst_doc:
            return "jsonst_doc";
        case jsonst_null:
            return "jsonst_null";
        case jsonst_true:
            return "jsonst_true";
        case jsonst_false:
            return "jsonst_false";
        case jsonst_num:
            return "jsonst_num";
        case jsonst_str:
            return "jsonst_str";
        case jsonst_arry:
            return "jsonst_arry";
        case jsonst_arry_elm:
            return "jsonst_arry_elm";
        case jsonst_arry_end:
            return "jsonst_arry_end";
        case jsonst_obj:
            return "jsonst_obj";
        case jsonst_obj_key:
            return "jsonst_obj_key";
        case jsonst_obj_end:
            return "jsonst_obj_end";
    }

    return JSONST_UNKNOWN;
}

const char *jsonst_error_to_str(const jsonst_error type) {
    switch (type) {
        case jsonst_success:
            return "jsonst_success";
        case jsonst_err_oom:
            return "jsonst_err_oom";
        case jsonst_err_str_buffer_full:
            return "jsonst_err_str_buffer_full";
        case jsonst_err_previous_error:
            return "jsonst_err_previous_error";
        case jsonst_err_internal_bug:
            return "jsonst_err_internal_bug";
        case jsonst_err_end_of_doc:
            return "jsonst_err_end_of_doc";
        case jsonst_err_invalid_eof:
            return "jsonst_err_invalid_eof";
        case jsonst_err_expected_new_value:
            return "jsonst_err_expected_new_value";
        case jsonst_err_unexpected_char:
            return "jsonst_err_unexpected_char";
        case jsonst_err_invalid_literal:
            return "jsonst_err_invalid_literal";
        case jsonst_err_invalid_control_char:
            return "jsonst_err_invalid_control_char";
        case jsonst_err_invalid_quoted_char:
            return "jsonst_err_invalid_quoted_char";
        case jsonst_err_invalid_hex_digit:
            return "jsonst_err_invalid_hex_digit";
        case jsonst_err_invalid_utf8_encoding:
            return "jsonst_err_invalid_utf8_encoding";
        case jsonst_err_invalid_number:
            return "jsonst_err_invalid_number";
        case jsonst_err_invalid_unicode_codepoint:
            return "jsonst_err_invalid_unicode_codepoint";
        case jsonst_err_invalid_utf16_surrogate:
            return "jsonst_err_invalid_utf16_surrogate";
        case jsonst_err_expected_new_key:
            return "jsonst_err_expected_new_key";
            break;
    }

    return JSONST_UNKNOWN;
}
