#ifndef JSONST_H
#define JSONST_H

// jsonst.{h,c}: The main implementation.

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Possible types of objects in a JSON document.
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

// A JSON value.
typedef struct {
    jsonst_type type;

    // Set only if type == jsonst_str or type == jsonst_num.
    //
    // In case of jsonst_num, you have to parse the number yourself - it is provided here exactly as
    // it is in the document. It is guaranteed to be a valid number as per JSON spec and strtod()
    // should always work on it. Parsing example:
    //
    //   #include <stdlib.h>
    //   char *endptr;
    //   double num = strtod(value->val_str.str, &endptr);
    //   // You might want to do real error handling here instead.
    //   assert(endptr == value->val_str.str + value->val_str.str_len);
    struct {
        // This is NULL byte terminated for compatibility with C strings.
        char* str;
        // Length of str _without_ the NULL byte.
        ptrdiff_t str_len;
    } val_str;
} jsonst_value;

// A JSON path describing the location of a value in a document.
typedef struct jsonst_path jsonst_path;
typedef struct jsonst_path {
    // Valid options are only jsonst_arry_elm and jsonst_obj_key.
    jsonst_type type;

    // Is NULL for the last path segment.
    jsonst_path* next;

    union {
        // Set if type == jsonst_arry_elm.
        uint32_t arry_ix;

        // Set if type == jsonst_obj_key.
        struct {
            // This is NULL byte terminated for compatibility with the C stdlib.
            char* str;
            // Length of str without NULL byte.
            ptrdiff_t str_len;
        } obj_key;
    } props;
} jsonst_path;

// Callback signature.
// The callback arguments and memory locations they point to have valid lifetime only until the
// callback returns. Excepted from that is user_data.
typedef void (*jsonst_value_cb)(void* user_data, const jsonst_value* value, const jsonst_path* p);

// Opaque handle to an instance.
// Use new_jsonst() to allocate one.
typedef struct _jsonst* jsonst;

// Default values for jsonst_config.
#define JSONST_DEFAULT_STR_ALLOC_BYTES (ptrdiff_t)128
#define JSONST_DEFAULT_OBJ_KEY_ALLOC_BYTES (ptrdiff_t)128
#define JSONST_DEFAULT_NUM_ALLOC_BYTES (ptrdiff_t)128

// Configuration values for jsonst.
// A zero-initialized struct is valid and means using the defaults.
typedef struct {
    // Max size in bytes for string values.
    ptrdiff_t str_alloc_bytes;
    // Max size in bytes for object keys.
    ptrdiff_t obj_key_alloc_bytes;
    // Max size in bytes for numbers before parsing.
    ptrdiff_t num_alloc_bytes;
} jsonst_config;

// Create an instance.
//
// - The instance will take ownership of the memory buffer mem (with size memsz) and use it as
// allocation arena.
// - It will not allocate memory by any other means (e.g. malloc).
// - To destroy the instance, simply free(mem).
// - An instance is good to parse one single JSON document.
// - To parse a new one, it needs to be reset. To do that, simply call new_jsonst() again with the
//   same mem argument.
//   Calling new_jsonst() with the same mem argument will always return the same instance struct
//   value, so you you can ignore it if all you want is to reset the instance.
// - Context can be passed to the callback by optionally passing non-NULL cb_user_data here.
//   It will be forwarded to the callback as first argument.
// - If the memory region mem passed in here is too small to allocate an instance, NULL is returned.
jsonst new_jsonst(uint8_t* mem, const ptrdiff_t memsz, const jsonst_value_cb cb, void* cb_user_data,
                  const jsonst_config conf);

// Error codes.
typedef enum {
    jsonst_success = 0,

    jsonst_err_oom,
    jsonst_err_str_buffer_full,
    jsonst_err_previous_error,
    jsonst_err_internal_bug,
    jsonst_err_end_of_doc,
    jsonst_err_invalid_eof,

    jsonst_err_expected_new_value,
    jsonst_err_expected_new_key,
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

// Feed the next byte to the parser.
// This can cause zero, one or multiple invocations of the callback function.
// At the end of your input, you must call this method once with c = JSONST_EOF.
// Returns jsonst_success or an error code.
jsonst_error jsonst_feed(jsonst j, const char c);

// Return value of jsonst_feed_doc.
typedef struct {
    jsonst_error err;
    size_t parsed_bytes;
} jsonst_feed_doc_ret;

// Feed an entire document to the parser.
// Will process until the document is entirely parsed, or an error is met.
// Returns jsonst_success or an error code, as well as the number of bytes the
// parser has read from the doc.
jsonst_feed_doc_ret jsonst_feed_doc(jsonst j, const char* doc, const size_t docsz);

#endif
