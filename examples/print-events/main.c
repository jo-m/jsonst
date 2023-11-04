#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/jsonst.h"
#include "src/jsonst_helpers.h"

static const char *doc = "{ \"key0\":[1,2,3,{\"key1\":false}] }";

void cb(const jsonst_value *value, const jsonst_path *p_first, const jsonst_path *p_last) {
    jsonst_path_print(p_first);
    printf("\n");
    jsonst_path_print_reverse(p_last);
    printf("\n");

    switch (value->type) {
        case jsonst_null:
            printf("jsonst_value_cb(%s)\n", jsonst_type_to_str(value->type));
            break;
        case jsonst_true:
            printf("jsonst_value_cb(%s)\n", jsonst_type_to_str(value->type));
            break;
        case jsonst_false:
            printf("jsonst_value_cb(%s)\n", jsonst_type_to_str(value->type));
            break;
        case jsonst_num:
            printf("jsonst_value_cb(%s, %f)\n", jsonst_type_to_str(value->type), value->val_num);
            break;
        case jsonst_str:
            printf("jsonst_value_cb(%s, '%.*s')\n", jsonst_type_to_str(value->type),
                   (int)value->val_str.str_len, value->val_str.str);
            break;
        case jsonst_obj_key:
            printf("jsonst_value_cb(%s, k='%.*s')\n", jsonst_type_to_str(value->type),
                   (int)value->val_str.str_len, value->val_str.str);
            break;
        case jsonst_arry:
            printf("jsonst_value_cb(%s, [)\n", jsonst_type_to_str(value->type));
            break;
        case jsonst_arry_end:
            printf("jsonst_value_cb(%s, ])\n", jsonst_type_to_str(value->type));
            break;
        case jsonst_obj:
            printf("jsonst_value_cb(%s, {)\n", jsonst_type_to_str(value->type));
            break;
        case jsonst_obj_end:
            printf("jsonst_value_cb(%s, })\n", jsonst_type_to_str(value->type));
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

    printf("%s\n", doc);

    jsonst j = new_jsonst(mem, memsz, cb);
    for (ptrdiff_t i = 0; doc[i] != 0; i++) {
        jsonst_feed(j, doc[i]);
    }
    jsonst_feed(j, EOF);

    free(mem);
    return 0;
}
