#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "src/jsons.h"

static const char *doc =
    " [ null , false  , \"hello there\"  ,true, {\"mykey\": false , \"another\": \"val\"}]";

void cb(const jsons_path *path, const jsons_value *value) {
    switch (value->type) {
        case json_null:
            printf("jsons_event_cb(%p, %d, null)\n", (void *)path, value->type);
            break;
        case json_true:
            printf("jsons_event_cb(%p, %d, true)\n", (void *)path, value->type);
            break;
        case json_false:
            printf("jsons_event_cb(%p, %d, false)\n", (void *)path, value->type);
            break;
        case json_num:
            printf("jsons_event_cb(%p, %d, %f)\n", (void *)path, value->type, value->num);
            break;
        case json_str:
            printf("jsons_event_cb(%p, %d, '%.*s')\n", (void *)path, value->type,
                   (int)value->str_len, value->str);
            break;
        case json_obj_key:
            printf("jsons_event_cb(%p, %d, k='%.*s')\n", (void *)path, value->type,
                   (int)value->str_len, value->str);
            break;
        case json_arry:
            printf("jsons_event_cb(%p, %d, [] beg or end)\n", (void *)path, value->type);
            break;
        case json_obj:
            printf("jsons_event_cb(%p, %d, {} beg or end)\n", (void *)path, value->type);
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

    return 0;
}
