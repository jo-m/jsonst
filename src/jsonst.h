#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    jsonst_doc = 'd',
    jsonst_null = 'n',
    jsonst_true = 't',
    jsonst_false = 'f',
    jsonst_num = 'x',
    jsonst_str = 's',

    jsonst_arry = '[',
    jsonst_arry_elm = 'e',
    jsonst_arry_end = ']',

    jsonst_obj = '{',
    jsonst_obj_key = 'k',
    jsonst_obj_end = '}',
} jsonst_type;

typedef struct {
    jsonst_type type;

    union {
        // Set if type == jsonst_num.
        double val_num;

        // Set if type == jsonst_str.
        struct {
            char* str;
            ptrdiff_t str_len;
        } val_str;
    };
} jsonst_value;

typedef struct jsonst_path jsonst_path;
typedef struct jsonst_path {
    // Valid options are only jsonst_arry_elm, jsonst_obj_key.
    jsonst_type type;

    // Will be NULL for last path segment.
    jsonst_path* next;

    union {
        // Set if type == jsonst_arry_elm.
        uint32_t arry_ix;

        // Set if type == jsonst_obj_key.
        struct {
            char* str;
            ptrdiff_t str_len;
        } obj_key;
    } props;
} jsonst_path;

// Callback signature.
// All arguments and locations they might point to have valid lifetime only until the callback
// returns.
typedef void (*jsonst_value_cb)(const jsonst_value* value, const jsonst_path* p);

// Opaque handle.
typedef struct _jsonst* jsonst;

// Will take ownership of mem (with size memsz) and use it for processing.
// Will not allocate any memory by itself.
// After the parser is done, simply free(mem).
// Might return NULL on OOM.
// To reset an instance to parse a new doc after EOF has been reached,
// simply call new_jsonst() again, with the same mem used previously.
jsonst new_jsonst(uint8_t* mem, const ptrdiff_t memsz, const jsonst_value_cb cb);

typedef enum {
    jsonst_success = 0,

    jsonst_err_oom,
    jsonst_err_str_buffer_full,
    jsonst_err_previous_error,
    jsonst_err_internal_bug,
    jsonst_err_end_of_doc,

    jsonst_err_expected_new_value,
    jsonst_err_unexpected_char,
    jsonst_err_invalid_literal,
    jsonst_err_invalid_control_char,
    jsonst_err_invalid_quoted_char,
    jsonst_err_invalid_hex_digit,
    jsonst_err_invalid_utf8_encoding,
    jsonst_err_invalid_number,
    jsonst_err_invalid_unicode_codepoint,
    jsonst_err_invalid_utf16_surrogate,
} jsonst_error;

#define JSONST_EOF (-1)

// At the end of your input, you must call this method once with c = JSONST_EOF.
jsonst_error jsonst_feed(jsonst j, const char c);

jsonst_error jsonst_feed_doc(jsonst j, const char* doc, const ptrdiff_t docsz);
