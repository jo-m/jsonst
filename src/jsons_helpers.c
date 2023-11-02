#include "jsons_helpers.h"

#include <stdlib.h>

const char *json_type_to_str(json_type type) {
    switch (type) {
        case json_doc:
            return "json_doc";
        case json_null:
            return "json_null";
        case json_true:
            return "json_true";
        case json_false:
            return "json_false";
        case json_num:
            return "json_num";
        case json_str:
            return "json_str";
        case json_arry:
            return "json_arry";
        case json_obj:
            return "json_obj";
        case json_obj_key:
            return "json_obj_key";
        case _json_end:
            break;
    }

    abort();
    return "__UNKNOWN__";
}
