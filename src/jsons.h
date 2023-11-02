#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "arena.h"

typedef enum {
    json_doc = 'd',
    json_null = '0',
    json_true = 't',
    json_false = 'f',
    json_num = '%',
    json_str = 's',

    json_arry = '[',      // TODO: expect value or ']'
    json_arry_elm = '*',  // TODO: expect ',' or ']'

    json_obj = '{',
    json_obj_key = '=',
} json_type;

// TODO: Move back to jsons_impl.h
// TODO: check that casting is only done where needed.
typedef enum {
    json_arry_elm_next = '+',  // TODO: expect value

    json_obj_post_key = ':',
    json_obj_val = 'v',
    json_obj_post_val = '?',
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
typedef void (*jsons_event_cb)(const jsons_path* path, const jsons_value* value);

typedef struct _json_streamer* json_streamer;

// will take ownership of the arena
json_streamer new_json_streamer(arena a, jsons_event_cb cb);

void json_streamer_feed(json_streamer j, char c);

void json_streamer_terminate(json_streamer j);
