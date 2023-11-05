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

// In most places where jsonst_type is used, jsonst_internal_state may also be used.
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
} jsonst_internal_state;

typedef struct frame frame;
struct frame {
    frame *prev;
    int type;  // Is a jsonst_type or jsonst_internal_state.
    arena a;

    uint32_t input_counter;

    // Processing state, usage depends on type.
    // Some types will use len separately without str.
    s8 str;
    ptrdiff_t len;
};

typedef struct _jsonst _jsonst;
typedef struct _jsonst {
    jsonst_value_cb cb;
    jsonst_error failed;
    frame *sp;
} _jsonst;

// We do not include <stdlib.h> so strtod remains configurable.
double strtod(const char *nptr, char **endptr);

// Helpers for error handling.
#define RET_ON_ERR(expr)                 \
    {                                    \
        const jsonst_error ret = (expr); \
        if (ret != jsonst_success) {     \
            return ret;                  \
        }                                \
    }

#define RET_OOM_IFNULL(expr)   \
    if ((expr) == NULL) {      \
        return jsonst_err_oom; \
    }
