#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/jsons.h"
#include "src/jsons_helpers.h"

// static const char *doc =
//     " [ null , false  , \"hello there\"  ,true, {\"mykey\": false , \"another\": \"val\"}, 123]";

static const char *doc = " [ true, false , null  ,[]  , 123, 123,true ] ";

void cb(const jsons_path *path, const jsons_value *value) {
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
        case json_obj_key:
            printf("jsons_event_cb(%p, %s, k='%.*s')\n", (void *)path,
                   json_type_to_str(value->type), (int)value->val_str.str_len, value->val_str.str);
            break;
        case json_arry:
            printf("jsons_event_cb(%p, %s, [])\n", (void *)path, json_type_to_str(value->type));
            break;
        case json_obj:
            printf("jsons_event_cb(%p, %s, {})\n", (void *)path, json_type_to_str(value->type));
            break;
        default:
            printf("unhandled %d", (int)value->type);
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
    json_streamer_terminate(j);

    arena_free(a);
    return 0;
}
