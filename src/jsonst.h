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

typedef struct jsonst_path jsonst_path;
typedef struct jsonst_path {
    // Valid options are only jsonst_arry_elm, jsonst_obj_key.
    jsonst_type type;

    // Will be NULL for last path segment.
    jsonst_path* next;
    // Will be NULL for first path segment.
    jsonst_path* prev;

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

// Callback signature.
typedef void (*jsonst_value_cb)(const jsonst_value* value, const jsonst_path* p_first,
                                const jsonst_path* p_last);

// Opaque handle.
typedef struct _jsonst* jsonst;

// Will take ownership of mem (with size memsz) and use it for processing.
// Will not allocate any memory by itself.
// After the parser is done, simply free(mem).
// Might return NULL on OOM.
// To reset an instance to parse a new doc after EOF has been reached,
// simply call new_jsonst() again, with the same mem used previously.
jsonst new_jsonst(uint8_t* mem, const ptrdiff_t memsz, jsonst_value_cb cb);

// At the end of your input, you MUST call this method once with c = EOF.
// Otherwise, it might fail to emit tokens at the end.
void jsonst_feed(jsonst j, char c);
