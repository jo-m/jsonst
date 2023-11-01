#include "jsons.h"

#include <stdio.h>   // TODO: remove
#include <stdlib.h>  // TODO: remove

#include "util.h"

#define STR_NULL "null"
#define STR_NULL_LEN (sizeof(STR_NULL) - 1)
#define STR_TRUE "true"
#define STR_TRUE_LEN (sizeof(STR_TRUE) - 1)
#define STR_FALSE "false"
#define STR_FALSE_LEN (sizeof(STR_FALSE) - 1)

// TODO: Eventually allow for dynamic realloc.
#define STR_MAX_LEN (ptrdiff_t)100

static void crash(const char *msg) {
    printf("\n============\n%s\n============\n", msg);
    abort();
}

// TODO: check that casting is only done where needed.
typedef enum {
    json_done = _json_end + 1,

    json_arry_post_elm,

    json_obj_post_key,
    json_obj_val,
    json_obj_post_val,
} json_internal_states;

typedef struct frame frame;
struct frame {
    frame *prev;
    int32_t type;

    // State of the frame.
    // Some types will use len separately without str.
    s8 str;
    ptrdiff_t len;
};

frame *new_frame(arena *a, frame *prev, json_type type) {
    frame *r = new (a, frame, 1, 0);
    r->prev = prev;
    r->type = type;
    return r;
}

static void frame_buf_putc(frame *f, const uint8_t c) {
    assert(f->str.buf != NULL);
    if (f->len >= f->str.len) {
        crash("frame_buf_putc: buffer full");
    }
    f->str.buf[f->len] = c;
    f->len++;
}

typedef struct _json_streamer _json_streamer;
typedef struct _json_streamer {
    arena a;  // TODO: Pass arena through frames so freeing also works...
    jsons_event_cb cb;

    frame *sp;
} _json_streamer;

_json_streamer *new__json_streamer(arena a, jsons_event_cb cb) {
    json_streamer r = new (&a, _json_streamer, 1, 0);
    r->a = a;
    assert(cb != NULL);
    r->cb = cb;
    r->sp = new_frame(&r->a, NULL, json_doc);
    return r;
}

// will take ownership of the arena
json_streamer new_json_streamer(arena a, jsons_event_cb cb) {
    _json_streamer *r = new__json_streamer(a, cb);
    assert(r->sp->type == json_doc);
    assert(r->sp->prev == NULL);
    return r;
}

static void emit(_json_streamer *j) {
    arena scratch = j->a;

    jsons_value *v = new (&scratch, jsons_value, 1, 0);
    v->type = j->sp->type;
    switch (j->sp->type) {
        case json_num:
            // TODO: actually parse buffer.
            v->num = 1337;
            break;
        case json_str:
            // TODO: maybe add nul byte?
            v->str = j->sp->str.buf;
            v->str_len = j->sp->str.len;
            break;
        case json_obj_key:
            // TODO: maybe add nul byte?
            v->str = j->sp->str.buf;
            v->str_len = j->sp->str.len;
            break;

            // Nothing else to do for the others.
    }

    jsons_path *p = new (&scratch, jsons_path, 1, 0);
    // TODO: fill in path!

    j->cb(p, v);
}

static bool is_ws(const uint8_t c) {
    switch (c) {
        case ' ':
        case '\n':
        case '\r':
        case '\t':
            return true;
    }
    return false;
}

static void push(_json_streamer *j, json_type type) {
    // printf("push(%d -> %d)\n", j->sp->type, type);
    j->sp = new_frame(&(j->a), j->sp, type);
}

static void pop(_json_streamer *j) {
    // printf("pop(%d)\n", j->sp->type);
    if (j->sp == NULL) {
        crash("pop: stack is empty");
    }

    const frame *popped = j->sp;
    j->sp = j->sp->prev;

    if (j->sp == NULL) {
        // Reached the end.
        return;
    }

    // We just ended an array element.
    if (j->sp->type == json_arry && popped->type != (int)json_arry_post_elm) {
        j->sp->len++;  // We count array elements here.

        push(j, (int)json_arry_post_elm);
        return;
    }

    // We just ended an object value.
    if (j->sp->type == json_obj_val && popped->type != (int)json_obj_post_val) {
        assert(j->sp->prev != NULL);
        assert(j->sp->prev->type == json_obj_key);
        assert(j->sp->prev->prev != NULL);
        assert(j->sp->prev->prev->type == json_obj);

        j->sp->prev->prev->len++;  // We count object elements here.

        pop(j);  // Pop json_obj_val.
        pop(j);  // Pop json_obj_key.

        push(j, (int)json_obj_post_val);
        return;
    }

    // We just ended the document.
    if (j->sp->type == json_doc) {
        pop(j);
        return;
    }
}

static void expect_value(_json_streamer *j, const uint8_t c) {
    if (is_ws(c)) {
        return;
    }

    switch (c) {
        case '[':
            push(j, json_arry);
            emit(j);
            return;
        case '{':
            push(j, json_obj);
            emit(j);
            return;
        case 'n':
            push(j, json_null);
            j->sp->len++;
            return;
        case 't':
            push(j, json_true);
            j->sp->len++;
            return;
        case 'f':
            push(j, json_false);
            j->sp->len++;
            return;
        case '"':
            push(j, json_str);
            j->sp->str = new_s8(&j->a, STR_MAX_LEN);
            return;
            // TODO: Handle json_num
        default:
            crash("expect_value: unhandled");
    }
}

static void feed(_json_streamer *j, const uint8_t c) {
    // printf("feed(%d, '%c')\n", peek_type(), c);

    if (j->sp == NULL) {
        if (is_ws(c)) {
            return;
        }
        crash("reached end of document");
    }

    switch (j->sp->type) {
        case json_doc:
            expect_value(j, c);
            return;
        case json_arry:
            if (c == ']') {
                emit(j);
                pop(j);
                return;
            }
            expect_value(j, c);
            return;
        case json_arry_post_elm:
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j);
                return;
            }
            if (c == ']') {
                pop(j);
                emit(j);
                pop(j);
                return;
            }
            crash("invalid, expected , or ]");
            return;
        case json_obj:
            if (is_ws(c)) {
                return;
            }
            if (c == '}') {
                emit(j);
                pop(j);
                return;
            }
            if (c == '"') {
                push(j, json_obj_key);
                j->sp->str = new_s8(&j->a, STR_MAX_LEN);
                return;
            }
            crash("invalid, expected \" or }");
            return;
        case json_obj_key:
            if (c == '"') {
                emit(j);
                push(j, json_obj_post_key);
                // We leave the key on the stack for now.
                return;
            }
            frame_buf_putc(j->sp, c);
            return;
        case json_obj_post_key:
            if (is_ws(c)) {
                return;
            }
            if (c == ':') {
                pop(j);
                push(j, json_obj_val);
                // We now still have the key right below.
                return;
            }
            crash("invalid: expected ':'");
            return;
        case json_obj_val:
            expect_value(j, c);
            return;
        case json_obj_post_val:
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j);
                return;
            }
            if (c == '}') {
                pop(j);
                emit(j);
                pop(j);
                return;
            }
            crash("invalid, expected , or }");
            return;
        case json_null:
            if (c != STR_NULL[j->sp->len]) {
                crash("invalid char, expected null literal");
            }
            j->sp->len++;
            if (j->sp->len == STR_NULL_LEN) {
                emit(j);
                pop(j);
            }
            return;
        case json_true:
            if (c != STR_TRUE[j->sp->len]) {
                crash("invalid char, expected true literal");
            }
            j->sp->len++;
            if (j->sp->len == STR_TRUE_LEN) {
                emit(j);
                pop(j);
            }
            return;
        case json_false:
            if (c != STR_FALSE[j->sp->len]) {
                crash("invalid char, expected false literal");
            }
            j->sp->len++;
            if (j->sp->len == STR_FALSE_LEN) {
                emit(j);
                pop(j);
            }
            return;
        case json_str:
            if (c == '"') {
                emit(j);
                pop(j);
                return;
            }
            // TODO: handle utf8 and escape sequences.
            frame_buf_putc(j->sp, c);
            return;
        default:
            printf("unhandled %d", (int)j->sp->type);
            crash("feed: unhandled");
    }
}

static void terminate(_json_streamer __attribute((unused)) * j) {
    assert(j->sp == NULL);
    // TODO: should also allow assert(j->sp == json_doc); here
}

// public entrypoints
void json_streamer_feed(json_streamer j, const uint8_t c) { feed(j, c); }
void json_streamer_terminate(json_streamer j) { terminate(j); }
