#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    json_doc = 'd',
    json_null = 'n',
    json_true = 't',
    json_false = 'f',
    json_num = 'x',
    json_str = 's',

    json_arry = '[',
    json_arry_elm = 'e',
    json_arry_end = ']',

    json_obj = '{',
    json_obj_name = 'k',
    json_obj_end = '}',
} json_type;

typedef struct jsons_path jsons_path;
typedef struct jsons_path {
    // Valid options are only json_arry, json_obj.
    json_type type;

    // Will be NULL for last entry.
    jsons_path* next;
    // Will be NULL for first entry.
    jsons_path* prev;

    union {
        // Set if type == json_arry.
        uint32_t arry_ix;

        // Set if type == json_obj.
        struct {
            char* str;
            ptrdiff_t str_len;
        } obj_key;
    } props;
} jsons_path;

typedef struct {
    json_type type;

    union {
        // Set if type == json_num.
        double val_num;

        // Set if type == json_str.
        struct {
            char* str;
            ptrdiff_t str_len;
        } val_str;
    };
} jsons_value;

// Callback signature.
typedef void (*jsons_event_cb)(const jsons_value* value, const jsons_path* path);

// Opaque handle.
typedef struct _json_streamer* json_streamer;

// Will take ownership of mem (with size memsz) and use it for processing.
// Will not allocate any memory by itself.
// After the parser is done, simply free(mem).
// Might return NULL on OOM.
json_streamer new_json_streamer(uint8_t* mem, const ptrdiff_t memsz, jsons_event_cb cb);

// At the end of your input, you MUST call this method once with c = EOF.
// Otherwise, it might fail to emit tokens at the end.
void json_streamer_feed(json_streamer j, char c);
