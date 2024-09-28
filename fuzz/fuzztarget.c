#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// A bit crude, but good enough for our purposes.
#include "../src/jsonst.c"
#include "../src/jsonst_util.c"
#include "jsonst_cstd.h"

#define DEFAULT_MEMSZ (8 * 1024)

void cb(__attribute((unused)) void *user_data, const jsonst_value *value, const jsonst_path *path) {
    // Path.
    printf("$");
    for (const jsonst_path *p = path; p != NULL; p = p->next) {
        switch (p->type) {
            case jsonst_arry_elm:
                printf("[%d]", p->props.arry_ix);
                break;
            case jsonst_obj_key:
                printf(".%.*s", (int)p->props.obj_key.str_len, p->props.obj_key.str);
                break;
            default:
                break;
        }
    }

    printf("=");

    // Value.
    switch (value->type) {
        case jsonst_null:
        case jsonst_true:
        case jsonst_false:
            printf("%s", jsonst_type_to_str(value->type));
            break;
        case jsonst_num:
            printf("(%s)%.*s", jsonst_type_to_str(value->type), (int)value->val_str.str_len,
                   value->val_str.str);
            break;
        case jsonst_str:
        case jsonst_obj_key:
            printf("(%s)'%.*s'", jsonst_type_to_str(value->type), (int)value->val_str.str_len,
                   value->val_str.str);
            break;
        case jsonst_arry:
        case jsonst_arry_end:
        case jsonst_obj:
        case jsonst_obj_end:
            printf("(%s)", jsonst_type_to_str(value->type));
            break;
        default:
            printf("%s=%c", jsonst_type_to_str(value->type), value->type);
    }

    printf("\n");
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: %s <json_file>\n", argv[0]);
        return 1;
    }

    FILE *f = fopen(argv[1], "r");
    if (f == NULL) {
        printf("Failed to open file '%s'", argv[0]);
        return 2;
    }

    uint8_t mem[DEFAULT_MEMSZ] = {0};
    jsonst_config conf = {0, 0, 0};
    jsonst j = new_jsonst(mem, DEFAULT_MEMSZ, cb, NULL, conf);
    if (j == NULL) {
        printf("Failed to initialize");
        return 2;
    }

    const jsonst_feed_doc_ret ret = jsonst_feed_fstream(j, f);
    printf("Result: %s\n", jsonst_error_to_str(ret.err));
}
