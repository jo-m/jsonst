#include "jsons.h"

#include <stdio.h>   // TODO: remove
#include <stdlib.h>  // TODO: remove

#include "jsons_impl.h"
#include "util.h"

// TODO: remove
static void visualize_stack(const frame *top) {
    if (top == NULL) {
        return;
    }
    visualize_stack(top->prev);
    printf("'%c' > ", top->type);
}

static void crash(const char *msg) {
    printf("\n============\n%s\n============\n", msg);
    abort();
}

static frame *new_frame(arena *a, frame *prev, json_type type) {
    frame *r = new (a, frame, 1, 0);
    r->prev = prev;
    r->type = type;
    return r;
}

static void frame_buf_putc(frame *f, const char c) {
    assert(f->str.buf != NULL);
    if (f->len >= f->str.len) {
        crash("frame_buf_putc: buffer full");
    }
    f->str.buf[f->len] = c;
    f->len++;
}

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
        case json_num: {
            char *endptr;
            v->val_num = strtod(j->sp->str.buf, &endptr);
            if (endptr != j->sp->str.buf + j->sp->len) {
                crash("failed to parse number or did not parse entire buffer");
            }
        } break;
        case json_str:
        case json_obj_key:
            v->val_str.str = j->sp->str.buf;
            v->val_str.str_len = j->sp->str.len;
            break;

            // Nothing else to do for the others.
    }

    jsons_path *p = new (&scratch, jsons_path, 1, 0);
    // TODO: fill in path!

    j->cb(p, v);
}

static bool is_ws(const char c) {
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
    visualize_stack(j->sp);
    printf("push('%c')\n", type);
    j->sp = new_frame(&(j->a), j->sp, type);
}

static void pop(_json_streamer *j, const int expect_type) {
    visualize_stack(j->sp);
    printf("pop('%c'=='%c')\n", j->sp->type, expect_type);
    if (j->sp == NULL) {
        crash("pop: stack is empty");
    }

    if (expect_type >= 0) {
        assert(j->sp->type == expect_type);
    }

    // const frame *popped = j->sp;
    j->sp = j->sp->prev;

    if (j->sp == NULL) {
        // Reached EOD.
        return;
    }

    // // We just ended an array element.
    // if (j->sp->type == json_arry_elm_next) {
    //     printf("ending array element\n");
    //     pop(j,c,json_arry_elm_next);
    //     assert(j->sp->type==json_arry);
    //     j->sp->len++;
    //     push(j,json_arry_elm);
    // }

    // // We just ended an array element.
    // if (j->sp->type == json_arry && popped->type != (int)json_arry_elm) {
    //     j->sp->len++;  // We count array elements here.

    //     push(j, (int)json_arry_elm);
    //     // if (c != ']') {
    //     //     push(j, (int)json_arry_elm);
    //     // } else {
    //     //     emit(j);
    //     //     pop(j, c,-1);
    //     // }

    //     return;
    // }

    // // We just ended an object value.
    // if (j->sp->type == json_obj_val && popped->type != (int)json_obj_post_val) {
    //     assert(j->sp->prev != NULL);
    //     assert(j->sp->prev->type == json_obj_key);
    //     assert(j->sp->prev->prev != NULL);
    //     assert(j->sp->prev->prev->type == json_obj);

    //     j->sp->prev->prev->len++;  // We count object elements here.

    //     pop(j, c,-1);  // Pop json_obj_val.
    //     pop(j, c,-1);  // Pop json_obj_key.

    //     push(j, (int)json_obj_post_val);
    //     // if (c != '}') {
    //     //     push(j, (int)json_obj_post_val);
    //     // } else {
    //     //     emit(j);
    //     //     pop(j, c,-1);
    //     // }

    //     return;
    // }

    // // We just ended the document.
    // if (j->sp->type == json_doc) {
    //     pop(j, c,json_doc);
    //     return;
    // }
}

bool is_num(const char c, const bool start_only) {
    if (c >= '0' && c <= '9') {
        return true;
    }
    if (c == '-') {
        return true;
    }

    if (start_only) {
        return false;
    }

    switch (c) {
        case '.':
        case 'e':
        case 'E':
        case '+':
            return true;
    }
    return false;
}

static void expect_value(_json_streamer *j, const char c) {
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
            j->sp->str = new_s8(&j->a, JSON_STR_MAX_LEN);
            return;
        default:
            if (is_num(c, true)) {
                push(j, json_num);
                j->sp->str = new_s8(&j->a, JSON_NUM_STR_MAX_LEN);
                frame_buf_putc(j->sp, c);
                return;
            }
            crash("expect_value: unexpected char");
    }
}

static void feed(_json_streamer *j, const char c) {
    visualize_stack(j->sp);
    printf("feed(\"%c\")\n", c);

    if (j->sp == NULL) {
        if (is_ws(c)) {
            return;
        }
        crash("reached end of document");
    }

    switch (j->sp->type) {
        case json_doc:
            if (is_ws(c)) {
                return;
            }
            expect_value(j, c);
            return;
        case json_arry:
            // TODO: json_arry = 10, // expect ws, value or ]
            if (is_ws(c)) {
                return;
            }
            if (c == ']') {
                emit(j);
                pop(j, json_arry);
                return;
            }
            // A value MUST follow now.
            push(j, (int)json_arry_elm);
            expect_value(j, c);
            return;
        case json_arry_elm:
            // TODO: json_arry_elm, // expect ws, ',' or ']'
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j, json_arry_elm);
                push(j, (int)json_arry_elm_next);
                return;
            }
            if (c == ']') {
                pop(j, json_arry_elm);
                emit(j);
                pop(j, json_arry);
                return;
            }
            crash("invalid, expected , or ]");
            return;
        case json_arry_elm_next:
            // TODO: json_arry_elm_next, 25 expect ws or value
            if (is_ws(c)) {
                return;
            }
            pop(j, json_arry_elm_next);
            push(j, json_arry_elm);
            expect_value(j, c);
            return;
        case json_obj:
            if (is_ws(c)) {
                return;
            }
            if (c == '}') {
                emit(j);
                pop(j, -1);
                return;
            }
            if (c == '"') {
                push(j, json_obj_key);
                j->sp->str = new_s8(&j->a, JSON_STR_MAX_LEN);
                return;
            }
            crash("invalid, expected \" or }");
            return;
        case json_obj_key:
            if (c == '"') {
                emit(j);
                push(j, (int)json_obj_post_key);
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
                pop(j, -1);
                push(j, (int)json_obj_val);
                // We now still have the key right below.
                return;
            }
            crash("invalid: expected ':'");
            return;
        case json_obj_val:
            if (is_ws(c)) {
                return;
            }
            expect_value(j, c);
            return;
        case json_obj_post_val:
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j, -1);
                return;
            }
            if (c == '}') {
                pop(j, -1);
                emit(j);
                pop(j, -1);
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
                pop(j, json_null);
            }
            return;
        case json_true:
            if (c != STR_TRUE[j->sp->len]) {
                crash("invalid char, expected true literal");
            }
            j->sp->len++;
            if (j->sp->len == STR_TRUE_LEN) {
                emit(j);
                pop(j, json_true);
            }
            return;
        case json_false:
            if (c != STR_FALSE[j->sp->len]) {
                crash("invalid char, expected false literal");
            }
            j->sp->len++;
            if (j->sp->len == STR_FALSE_LEN) {
                emit(j);
                pop(j, json_false);
            }
            return;
        case json_str:
            if (c == '"') {
                emit(j);
                pop(j, json_str);
                return;
            }
            // TODO: handle utf8 and escape sequences.
            frame_buf_putc(j->sp, c);
            return;
        case json_num:
            if (!is_num(c, false)) {
                emit(j);
                pop(j, json_num);

                // Numbers are the only type delimited by a char which already belongs to the next
                // token. Thus, we have feed that token to the parser again.
                feed(j, c);

                return;
            }
            // TODO: be more strict about structure.
            frame_buf_putc(j->sp, c);
            return;
        default:
            printf("unhandled %d", (int)j->sp->type);
            crash("feed: unhandled");
    }
}

static void terminate(_json_streamer __attribute((unused)) * j) {
    if (j->sp->type == json_doc) {
        j->sp = NULL;
    }
    assert(j->sp == NULL);
}

// public entrypoints
// TODO: proper error handling - return errors instead of crashing.
void json_streamer_feed(json_streamer j, const char c) { feed(j, c); }
void json_streamer_terminate(json_streamer j) { terminate(j); }
