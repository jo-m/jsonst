#pragma once

#include "util.h"

#define STR_NULL "null"
#define STR_NULL_LEN (sizeof(STR_NULL) - 1)
#define STR_TRUE "true"
#define STR_TRUE_LEN (sizeof(STR_TRUE) - 1)
#define STR_FALSE "false"
#define STR_FALSE_LEN (sizeof(STR_FALSE) - 1)

#define STR_ALLOC (ptrdiff_t)128
#define NUM_STR_ALLOC (ptrdiff_t)64

// In most places where jsonst_type is used, jsonst_internal_states may also be used.
// Thus, we need to be careful to no accidentally introduce overlaps.
typedef enum {
    jsonst_arry_elm_next = '+',

    jsonst_obj_post_name = ':',
    jsonst_obj_post_sep = '_',
    jsonst_obj_next = '-',

    jsonst_str_utf8 = '8',
    jsonst_str_escp = '\\',
    jsonst_str_escp_uhex = '%',
    jsonst_str_escp_uhex_utf16 = '6',
} jsonst_internal_states;

typedef struct frame frame;
struct frame {
    frame *prev;
    int type;  // Is a jsonst_type or jsonst_internal_states.
    arena a;

    // Processing state, usage depends on type.
    // Some types will use len separately without str.
    s8 str;
    ptrdiff_t len;
};

typedef struct _jsonst _jsonst;
typedef struct _jsonst {
    jsonst_value_cb cb;

    frame *sp;
} _jsonst;
