#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "arena.h"

// TODO: change back to numbers
typedef enum {
    json_doc = 'd',
    json_null = '0',
    json_true = 't',
    json_false = 'f',
    json_num = 'x',  // TODO: expect - or 0-9
    json_str = 's',

    json_arry = '[',
    json_arry_elm = '*',
    json_arry_end = ']',

    json_obj = '{',
    json_obj_key = 'k',
    json_obj_end = '}',
} json_type;

// TODO: Move back to jsons_impl.h
// TODO: check that casting is only done where needed.
// TODO: rename array and obj enums to be consistent.
typedef enum {
    json_arry_elm_next = '+',

    json_obj_post_key = ':',
    json_obj_post_colon = '_',
    json_obj_next = 'v',

    json_str_utf8 = '8',
    json_str_escp = '\\',
    json_str_escp_uhex = 'u',
    json_str_escp_uhex_utf16 = '6',
} json_internal_states;

typedef struct jsons_path jsons_path;
typedef struct jsons_path {
    // Valid options are only json_arry, json_obj.
    json_type type;
    // Will be NULL for last entry.
    jsons_path* next;

    union {
        // Set if type == json_arry.
        int arry_ix;

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

typedef struct _json_streamer* json_streamer;

// will take ownership of the arena
// TODO: allow configuring some things, e.g. json path on/off.
// TODO: provide c++ API wrapper.
json_streamer new_json_streamer(arena a, jsons_event_cb cb);

// At the end of your input, you MUST call this method with c=0
// to signal EOF to the parser.
void json_streamer_feed(json_streamer j, char c);

// TODO: allow to reset somehow, also consider arena semantics
