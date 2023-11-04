#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/jsons.h"
#include "src/jsons_helpers.h"

static const char *doc = "{ \"aa \\\\ ää  \\\" \\r \\n aa\":123 }";

void cb(const jsons_value *value, const jsons_path *path) {
    switch (value->type) {
        case json_null:
            printf("jsons_event_cb(%p, %s)\n", (void *)path, json_type_to_str(value->type));
            break;
        case json_true:
            printf("jsons_event_cb(%p, %s)\n", (void *)path, json_type_to_str(value->type));
            break;
        case json_false:
            printf("jsons_event_cb(%p, %s)\n", (void *)path, json_type_to_str(value->type));
            break;
        case json_num:
            printf("jsons_event_cb(%p, %s, %f)\n", (void *)path, json_type_to_str(value->type),
                   value->val_num);
            break;
        case json_str:
            printf("jsons_event_cb(%p, %s, '%.*s')\n", (void *)path, json_type_to_str(value->type),
                   (int)value->val_str.str_len, value->val_str.str);
            break;
        case json_obj_name:
            printf("jsons_event_cb(%p, %s, k='%.*s')\n", (void *)path,
                   json_type_to_str(value->type), (int)value->val_str.str_len, value->val_str.str);
            break;
        case json_arry:
            printf("jsons_event_cb(%p, %s, [)\n", (void *)path, json_type_to_str(value->type));
            break;
        case json_arry_end:
            printf("jsons_event_cb(%p, %s, ])\n", (void *)path, json_type_to_str(value->type));
            break;
        case json_obj:
            printf("jsons_event_cb(%p, %s, {)\n", (void *)path, json_type_to_str(value->type));
            break;
        case json_obj_end:
            printf("jsons_event_cb(%p, %s, })\n", (void *)path, json_type_to_str(value->type));
            break;
        default:
            printf("unhandled %d\n", (int)value->type);
            abort();
    }
}

int main(void) {
    arena a = new_arena(10000);
    assert(a.beg != NULL);

    json_streamer j = new_json_streamer(a, cb);
    for (ptrdiff_t i = 0; doc[i] != 0; i++) {
        json_streamer_feed(j, doc[i]);
    }
    json_streamer_feed(j, 0);

    arena_free(a);
    return 0;
}
