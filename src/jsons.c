#include "jsons.h"

#include <stdarg.h>  // TODO: remove
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

static void crash(const char *format, ...) {
    printf("\n============ CRASH ============\n");

    va_list args;
    va_start(args, format);
    vprintf(format, args);

    va_end(args);

    printf("\n============ /CRASH ===========\n");
    abort();
}

static frame *new_frame(arena *a, frame *prev, const json_type type) {
    frame *r = new (a, frame, 1, 0);
    r->prev = prev;
    r->type = type;
    return r;
}

static void frame_buf_putc(frame *f, const char c) {
    assert(f->str.buf != NULL);
    if (f->len >= f->str.len) {
        crash("frame_buf_putc(): buffer full");
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

// TODO: start/end
static void emit(_json_streamer *j) {
    arena scratch = j->a;

    jsons_value *v = new (&scratch, jsons_value, 1, 0);
    v->type = j->sp->type;
    switch (j->sp->type) {
        case json_num: {
            char *endptr;
            v->val_num = strtod(j->sp->str.buf, &endptr);
            if (endptr != j->sp->str.buf + j->sp->len) {
                crash("failed to parse number or did not parse entire buffer '%.*s'", j->sp->len,
                      j->sp->str.buf);
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

static bool is_digit(const char c) { return c >= '0' && c <= '9'; }

static bool is_xdigit(const char c) {
    if (is_digit(c)) {
        return true;
    }
    if (c >= 'a' && c <= 'f') {
        return true;
    }
    if (c >= 'A' && c <= 'F') {
        return true;
    }
    return false;
}

static int32_t conv_xdigit(const char c) {
    if (is_digit(c)) {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    return -1;
}

// As per JSON spec.
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

static bool is_num(const char c) {
    if (is_digit(c)) {
        return true;
    }
    switch (c) {
        case '.':
        case 'e':
        case 'E':
        case '+':
        case '-':
            return true;
    }
    return false;
}

static bool is_cntrl(const char c) { return (c >= 0x00 && c <= 0x1F) || c == 0x7F; }

static bool is_utf16_high_surrogate(const uint32_t c) { return c >= 0xD800 && c < 0xDC00; }

static bool is_utf16_low_surrogate(const uint32_t c) { return c >= 0xDC00 && c < 0xE000; }

// Str must be of length 4, and characters must all be hex digits.
static uint32_t parse_hex4(const s8 str) {
    const ptrdiff_t len = 4;
    assert(str.len == len);

    uint32_t ret = 0;
    for (int i = 0; i < len; i++) {
        unsigned char c = str.buf[i];
        assert(is_xdigit(c));

        ret <<= 4;
        ret |= (uint32_t)conv_xdigit(c);
    }

    return ret;
}

static void utf8_encode(frame *f, uint32_t c) {
    if (c <= 0x7F) {
        frame_buf_putc(f, c);
        return;
    } else if (c <= 0x07FF) {
        frame_buf_putc(f, (((c >> 6) & 0x1F) | 0xC0));
        frame_buf_putc(f, (((c >> 0) & 0x3F) | 0x80));
        return;
    } else if (c <= 0xFFFF) {
        frame_buf_putc(f, (((c >> 12) & 0x0F) | 0xE0));
        frame_buf_putc(f, (((c >> 6) & 0x3F) | 0x80));
        frame_buf_putc(f, (((c >> 0) & 0x3F) | 0x80));
        return;
    } else if (c <= 0x10FFFF) {
        frame_buf_putc(f, (((c >> 18) & 0x07) | 0xF0));
        frame_buf_putc(f, (((c >> 12) & 0x3F) | 0x80));
        frame_buf_putc(f, (((c >> 6) & 0x3F) | 0x80));
        frame_buf_putc(f, (((c >> 0) & 0x3F) | 0x80));
        return;
    }
    crash("could not encode as utf-8: '%c' = %d", c, c);
}

static void push(_json_streamer *j, const json_type type) {
    visualize_stack(j->sp);
    printf("push('%c')\n", type);
    j->sp = new_frame(&(j->a), j->sp, type);
}

static void pop(_json_streamer *j, const json_type expect_type) {
    visualize_stack(j->sp);
    printf("pop('%c')\n", j->sp->type);
    if (j->sp == NULL) {
        crash("pop: stack is empty");
    }

    assert(j->sp->type == (int)expect_type);

    j->sp = j->sp->prev;

    if (j->sp == NULL) {
        // Reached EOD.
        return;
    }
}

static void expect_start_value(_json_streamer *j, const char c) {
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
            if (is_digit(c) || c == '-') {
                push(j, json_num);
                j->sp->str = new_s8(&j->a, JSON_NUM_STR_MAX_LEN);
                frame_buf_putc(j->sp, c);
                return;
            }
            crash("expect_start_value(): unexpected char '%c'", c);
    }
}

static char is_quotable(const char c) {
    switch (c) {
        case '"':
            return c;
        case '\\':
            return c;
        case '/':
            return c;
        case 'b':
            return '\b';
        case 'f':
            return '\f';
        case 'n':
            return '\n';
        case 'r':
            return '\r';
        case 't':
            return '\t';
    }
    return 0;
}

static void feed(_json_streamer *j, const char c) {
    visualize_stack(j->sp);
    printf("feed(\"%c\" = %d)\n", c, c);

    // EOF, with special treatment for numbers (which have no delimiters themselves).
    if (c == 0 && j->sp->type != json_num) {
        if (j->sp->type == json_doc) {
            j->sp = NULL;
        }
        assert(j->sp == NULL);
        return;
    }

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
            expect_start_value(j, c);
            return;
        case json_arry:
            if (is_ws(c)) {
                return;
            }
            if (c == ']') {
                emit(j);
                pop(j, json_arry);
                return;
            }
            push(j, json_arry_elm);
            expect_start_value(j, c);
            return;
        case json_arry_elm:
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j, json_arry_elm);
                assert(j->sp->type == json_arry);
                push(j, (int)json_arry_elm_next);
                return;
            }
            if (c == ']') {
                pop(j, json_arry_elm);
                emit(j);
                pop(j, json_arry);
                return;
            }
            crash("invalid char '%c', expected ',' or ']'", c);
            return;
        case json_arry_elm_next:
            if (is_ws(c)) {
                return;
            }
            pop(j, (int)json_arry_elm_next);
            assert(j->sp->type == json_arry);
            push(j, json_arry_elm);
            expect_start_value(j, c);
            return;
        case json_obj:
            if (is_ws(c)) {
                return;
            }
            if (c == '}') {
                emit(j);
                pop(j, json_obj);
                return;
            }
            if (c == '"') {
                push(j, json_obj_key);
                j->sp->str = new_s8(&j->a, JSON_STR_MAX_LEN);
                return;
            }
            crash("invalid char '%c', expected '\"' or '}'", c);
            return;
        case json_obj_key:
            // TODO: escaping etc.
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
                pop(j, (int)json_obj_post_key);
                assert(j->sp->type == json_obj_key);
                push(j, (int)json_obj_post_colon);
                // We now still have the key on the stack one below.
                return;
            }
            crash("invalid char '%c': expected ':'", c);
            return;
        case json_obj_post_colon:
            if (is_ws(c)) {
                return;
            }
            pop(j, (int)json_obj_post_colon);
            assert(j->sp->type == json_obj_key);
            push(j, (int)json_obj_next);
            expect_start_value(j, c);
            return;
        case json_obj_next:
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j, (int)json_obj_next);
                pop(j, json_obj_key);
                assert(j->sp->type == json_obj);
                return;
            }
            if (c == '}') {
                pop(j, (int)json_obj_next);
                pop(j, json_obj_key);
                emit(j);
                pop(j, json_obj);
                return;
            }
            crash("invalid char '%c', expected ',' or '}'", c);
            return;
        case json_null:
            if (c != STR_NULL[j->sp->len]) {
                crash("invalid char '%c', expected 'null' literal", c);
            }
            j->sp->len++;
            if (j->sp->len == STR_NULL_LEN) {
                emit(j);
                pop(j, json_null);
            }
            return;
        case json_true:
            if (c != STR_TRUE[j->sp->len]) {
                crash("invalid char '%c', expected 'true' literal", c);
            }
            j->sp->len++;
            if (j->sp->len == STR_TRUE_LEN) {
                emit(j);
                pop(j, json_true);
            }
            return;
        case json_false:
            if (c != STR_FALSE[j->sp->len]) {
                crash("invalid char '%c', expected 'false' literal", c);
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
            if (c == '\\') {
                // TODO: handle escape.
                push(j, (int)json_str_escp);
                return;
            }
            // Single byte, 0xxxxxxx.
            if (((unsigned char)c & 0x80) == 0) {
                if (is_cntrl(c)) {
                    crash("unexpected control character 0x%x", c);
                }
                frame_buf_putc(j->sp, c);
                return;
            }
            // 2 bytes, 110xxxxx.
            if (((unsigned char)c & 0xE0) == 0xC0) {
                frame_buf_putc(j->sp, c);
                push(j, (int)json_str_utf8);
                j->sp->len = 1;  // Remaining bytes for this codepoint.
                return;
            }
            // 3 bytes, 1110xxxx.
            if (((unsigned char)c & 0xF0) == 0xE0) {
                frame_buf_putc(j->sp, c);
                push(j, (int)json_str_utf8);
                j->sp->len = 2;  // Remaining bytes for this codepoint.
                return;
            }
            // 4 bytes, 11110xxx.
            if (((unsigned char)c & 0xF8) == 0xF0) {
                frame_buf_putc(j->sp, c);
                push(j, (int)json_str_utf8);
                j->sp->len = 3;  // Remaining bytes for this codepoint.
                return;
            }
            crash("unhandled string char 0x%x", c);
            return;
        case json_str_escp:
            if (c == 'u') {
                push(j, (int)json_str_escp_uhex);
                j->sp->str = new_s8(&j->a, 4);
                return;
            }
            const char unquoted = is_quotable(c);
            if (unquoted != 0) {
                // Write to underlying string buffer.
                assert(j->sp->prev->type == json_str);
                frame_buf_putc(j->sp->prev, unquoted);
                pop(j, (int)json_str_escp);
                return;
            }
            crash("invalid quoted char %c 0x%x", c, c);
            return;
        case json_str_utf8:
            // Check that conforms to 10xxxxxx.
            if (((unsigned char)c & 0xC0) != 0x80) {
                crash("invalid utf8-encoding, expected 0b10xxxxxx but got 0x%x", c);
            }
            // Write to underlying string buffer.
            assert(j->sp->prev->type == json_str);
            frame_buf_putc(j->sp->prev, c);

            j->sp->len--;
            if (j->sp->len == 0) {
                pop(j, (int)json_str_utf8);
            }
            return;
        case json_str_escp_uhex:
            assert(j->sp->prev->type == json_str_escp);
            assert(j->sp->prev->prev->type == json_str);
            if (!is_xdigit(c)) {
                crash("expected hex digit but got '%c'", c);
                return;
            }
            frame_buf_putc(j->sp, c);
            if (j->sp->len < 4) {
                return;
            }

            const uint32_t n = parse_hex4(j->sp->str);
            if (is_utf16_high_surrogate(n)) {
                push(j, (int)json_str_escp_uhex_utf16);
                j->sp->str = new_s8(&j->a, 4);
                j->sp->len = -2;  // Eat the \u sequence
            } else {
                utf8_encode(j->sp->prev->prev, n);
                pop(j, (int)json_str_escp_uhex);
                pop(j, (int)json_str_escp);
            }
            return;
        case json_str_escp_uhex_utf16:
            assert(j->sp->prev->type == json_str_escp_uhex);
            assert(j->sp->prev->prev->type == json_str_escp);
            assert(j->sp->prev->prev->prev->type == json_str);

            if (j->sp->len == -2 && c == '\\') {
                j->sp->len++;
                return;
            }
            if (j->sp->len == -1 && c == 'u') {
                j->sp->len++;
                return;
            }
            if (j->sp->len < 0) {
                crash("expected 2nd surrogate pair member but received '%c' = %d", c);
                return;
            }

            if (!is_xdigit(c)) {
                crash("expected hex digit but got '%c'", c);
                return;
            }
            frame_buf_putc(j->sp, c);
            if (j->sp->len < 4) {
                return;
            }

            {
                const uint32_t low = parse_hex4(j->sp->str);
                if (!is_utf16_low_surrogate(low)) {
                    crash("not a utf16 low surrogate: %d", low);
                }
                const uint32_t high = parse_hex4(j->sp->prev->str);
                assert(is_utf16_high_surrogate(high));

                const uint32_t decoded = ((high - 0xd800) << 10 | (low - 0xdc00)) + 0x10000;

                utf8_encode(j->sp->prev->prev->prev, decoded);
                pop(j, (int)json_str_escp_uhex_utf16);
                pop(j, (int)json_str_escp_uhex);
                pop(j, (int)json_str_escp);
                return;
            }
        case json_num:
            if (!is_num(c)) {
                emit(j);
                pop(j, json_num);

                // Numbers are the only type delimited by a char which already belongs to the next
                // token. Thus, we have feed that token to the parser again.
                feed(j, c);

                return;
            }
            // TODO: do proper parsing instead of just accepting everything and leaving the work to
            // stdlib.
            frame_buf_putc(j->sp, c);
            return;
        default:
            crash("feed(): unhandled type '%c' = %d", j->sp->type, j->sp->type);
    }
}

// public entrypoints
// TODO: proper error handling - return errors instead of crashing.
void json_streamer_feed(json_streamer j, const char c) { feed(j, c); }
