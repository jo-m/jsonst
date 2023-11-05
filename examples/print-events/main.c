#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/jsonst.h"
#include "src/jsonst_util.h"

static const char doc[] = "{ \"key0\":[1,2,3,{\"key1\":false}] }";

static void jsonst_path_print(const jsonst_path *path) {
    printf("$");
    for (const jsonst_path *p = path; p != NULL; p = p->next) {
        switch (p->type) {
            case jsonst_arry_elm:
                printf("[%d]", p->props.arry_ix);
                break;
            case jsonst_obj_key:
                printf("['%.*s']", (int)p->props.obj_key.str_len, p->props.obj_key.str);
                break;
            default:
                break;
        }
    }
}

static void cb(const jsonst_value *value, const jsonst_path *p) {
    jsonst_path_print(p);
    printf("\n");

    switch (value->type) {
        case jsonst_null:
        case jsonst_true:
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
        case jsonst_arry_end:
        case jsonst_obj:
        case jsonst_obj_end:
            printf("jsonst_value_cb(%s, %c)\n", jsonst_type_to_str(value->type), value->type);
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
    const jsonst_feed_doc_ret ret = jsonst_feed_doc(j, doc, sizeof(doc) - 1);
    assert(ret.err == jsonst_success);

    free(mem);
    return EXIT_SUCCESS;
}
