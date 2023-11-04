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

typedef struct frame frame;
struct frame {
    frame *prev;
    int type;

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
