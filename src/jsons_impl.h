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

// TODO: check that casting is only done where needed.
typedef enum {
    json_done = _json_end + 1,

    json_arry_post_elm,

    json_obj_post_key,
    json_obj_val,
    json_obj_post_val,
} json_internal_states;

typedef struct frame frame;
struct frame {
    frame *prev;
    int32_t type;

    // State of the frame.
    // Some types will use len separately without str.
    s8 str;
    ptrdiff_t len;
};

typedef struct _json_streamer _json_streamer;
typedef struct _json_streamer {
    arena a;  // TODO: Pass arena through frames so freeing also works...
    jsons_event_cb cb;

    frame *sp;
} _json_streamer;
