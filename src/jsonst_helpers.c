#include "jsonst_helpers.h"

#include <stdlib.h>

const char *jsonst_type_to_str(jsonst_type type) {
    switch (type) {
        case jsonst_doc:
            return "jsonst_doc";
        case jsonst_null:
            return "jsonst_null";
        case jsonst_true:
            return "jsonst_true";
        case jsonst_false:
            return "jsonst_false";
        case jsonst_num:
            return "jsonst_num";
        case jsonst_str:
            return "jsonst_str";
        case jsonst_arry:
            return "jsonst_arry";
        case jsonst_arry_elm:
            return "jsonst_arry_elm";
        case jsonst_arry_end:
            return "jsonst_arry_end";
        case jsonst_obj:
            return "jsonst_obj";
        case jsonst_obj_name:
            return "jsonst_obj_name";
        case jsonst_obj_end:
            return "jsonst_obj_end";
    }

    return "__UNKNOWN__";
}
