#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/jsonst.h"
#include "src/jsonst_helpers.h"

static const char *doc = "{ \"aa \\\\ ää  \\\" \\r \\n aa\":123 }";

void cb(const jsonst_value *value, const jsonst_path *path) {
    switch (value->type) {
        case jsonst_null:
            printf("jsonst_value_cb(%p, %s)\n", (void *)path, jsonst_type_to_str(value->type));
            break;
        case jsonst_true:
            printf("jsonst_value_cb(%p, %s)\n", (void *)path, jsonst_type_to_str(value->type));
            break;
        case jsonst_false:
            printf("jsonst_value_cb(%p, %s)\n", (void *)path, jsonst_type_to_str(value->type));
            break;
        case jsonst_num:
            printf("jsonst_value_cb(%p, %s, %f)\n", (void *)path, jsonst_type_to_str(value->type),
                   value->val_num);
            break;
        case jsonst_str:
            printf("jsonst_value_cb(%p, %s, '%.*s')\n", (void *)path,
                   jsonst_type_to_str(value->type), (int)value->val_str.str_len,
                   value->val_str.str);
            break;
        case jsonst_obj_name:
            printf("jsonst_value_cb(%p, %s, k='%.*s')\n", (void *)path,
                   jsonst_type_to_str(value->type), (int)value->val_str.str_len,
                   value->val_str.str);
            break;
        case jsonst_arry:
            printf("jsonst_value_cb(%p, %s, [)\n", (void *)path, jsonst_type_to_str(value->type));
            break;
        case jsonst_arry_end:
            printf("jsonst_value_cb(%p, %s, ])\n", (void *)path, jsonst_type_to_str(value->type));
            break;
        case jsonst_obj:
            printf("jsonst_value_cb(%p, %s, {)\n", (void *)path, jsonst_type_to_str(value->type));
            break;
        case jsonst_obj_end:
            printf("jsonst_value_cb(%p, %s, })\n", (void *)path, jsonst_type_to_str(value->type));
            break;
        default:
            printf("unhandled %d\n", (int)value->type);
            abort();
    }
}

int main(void) {
    const ptrdiff_t memsz = 1024 * 8;
    void *mem = malloc(memsz);
    assert(mem != NULL);

    jsonst j = new_jsonst(mem, memsz, cb);
    for (ptrdiff_t i = 0; doc[i] != 0; i++) {
        jsonst_feed(j, doc[i]);
    }
    jsonst_feed(j, EOF);

    free(mem);
    return 0;
}
