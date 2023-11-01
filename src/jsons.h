#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "arena.h"

typedef enum {
    json_doc = 0,
    json_null = 1,
    json_true = 2,
    json_false = 3,
    json_num = 4,
    json_str = 5,

    json_arry = 10,

    json_obj = 20,
    json_obj_key = 21,
    _json_end,
} json_type;

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
            uint8_t* str;
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
            uint8_t* str;
            ptrdiff_t str_len;
        } val_str;
    };
} jsons_value;

// Callback signature.
typedef void (*jsons_event_cb)(const jsons_path* path, const jsons_value* value);

typedef struct _json_streamer* json_streamer;

// will take ownership of the arena
json_streamer new_json_streamer(arena a, jsons_event_cb cb);

void json_streamer_feed(json_streamer j, uint8_t c);

void json_streamer_terminate(json_streamer j);
