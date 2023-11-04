#pragma once

#include "util.h"

#define STR_NULL "null"
#define STR_NULL_LEN (sizeof(STR_NULL) - 1)
#define STR_TRUE "true"
#define STR_TRUE_LEN (sizeof(STR_TRUE) - 1)
#define STR_FALSE "false"
#define STR_FALSE_LEN (sizeof(STR_FALSE) - 1)

// TODO: Eventually allow for dynamic realloc.
#define JSON_STR_MAX_LEN (ptrdiff_t)100
#define JSON_NUM_STR_MAX_LEN (ptrdiff_t)100

// In most places where json_type is used, json_internal_states may also be used.
// Thus, we need to be careful to no accidentally introduce overlaps.
typedef enum {
    json_arry_elm_next = '+',

    json_obj_post_name = ':',
    json_obj_post_sep = '_',
    json_obj_next = '-',

    json_str_utf8 = '8',
    json_str_escp = '\\',
    json_str_escp_uhex = '%',
    json_str_escp_uhex_utf16 = '6',
} json_internal_states;

typedef struct frame frame;
struct frame {
    frame *prev;
    int type;  // Is a json_type or json_internal_states.

    // State of the frame.
    // Some types will use len separately without str.
    s8 str;
    ptrdiff_t len;
};

typedef struct _json_streamer _json_streamer;
typedef struct _json_streamer {
    arena a;
    jsons_event_cb cb;

    frame *sp;
} _json_streamer;
