#include "jsonst.h"

#include <stdarg.h>  // TODO: remove
#include <stdio.h>   // TODO: remove
#include <stdlib.h>  // TODO: remove

#include "jsonst_impl.h"
#include "util.h"

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

static frame *new_frame(arena *a, frame *prev,
                        const jsonst_type /* or jsonst_internal_states */ type) {
    // Allocate the frame itself on the parent arena.
    frame *f = new (a, frame, 1, 0);
    if (f == NULL) {
        return NULL;
    }

    f->prev = prev;
    f->type = type;
    // We now make a copy.
    f->a = *a;

    return f;
}

_jsonst *new__jsonst(uint8_t *mem, const ptrdiff_t memsz, jsonst_value_cb cb) {
    arena a = new_arena(mem, memsz);
    jsonst j = new (&a, _jsonst, 1, 0);
    if (j == NULL) {
        return NULL;
    }

    assert(cb != NULL);
    j->cb = cb;

    j->sp = new_frame(&a, NULL, jsonst_doc);
    if (j->sp == NULL) {
        return NULL;
    }

    return j;
}

jsonst new_jsonst(uint8_t *mem, const ptrdiff_t memsz, jsonst_value_cb cb) {
    return new__jsonst(mem, memsz, cb);
}

static void frame_buf_putc(frame *f, const char c) {
    assert(f->str.buf != NULL);
    if (f->len >= f->str.len) {
        crash("frame_buf_putc(): buffer full");
    }
    f->str.buf[f->len] = c;
    f->len++;
}

static void emit(_jsonst *j, const jsonst_type /* or jsonst_internal_states */ type) {
    arena scratch = j->sp->a;

    jsonst_value *v = new (&scratch, jsonst_value, 1, 0);
    if (v == NULL) {
        crash("OOM");
    }
    v->type = type;
    switch (type) {
        case jsonst_num: {
            assert(j->sp->type == jsonst_num);
            char *endptr;
            v->val_num = strtod(j->sp->str.buf, &endptr);
            if (endptr != j->sp->str.buf + j->sp->len) {
                crash("failed to parse number or did not parse entire buffer '%.*s'", j->sp->len,
                      j->sp->str.buf);
            }
        } break;
        case jsonst_str:
        case jsonst_obj_key:
            assert(j->sp->type == jsonst_str || j->sp->type == jsonst_obj_key);
            v->val_str.str = j->sp->str.buf;
            v->val_str.str_len = j->sp->str.len;
            break;
        default:
            // Nothing else to do for the others.
            break;
    }

    jsonst_path *p_first = NULL, *p_last = NULL;
    // TODO: actually fill in arry_ix for arrays
    for (const frame *f = j->sp; f != NULL; f = f->prev) {
        switch (f->type) {
            case jsonst_arry_elm:
            case jsonst_obj_key:
                break;
            default:
                continue;
        }

        jsonst_path *p_new = new (&scratch, jsonst_path, 1, 0);
        if (p_new == NULL) {
            crash("OOM");
        }
        p_new->type = f->type;

        switch (f->type) {
            case jsonst_arry_elm:
                assert(f->prev->type == jsonst_arry);
                p_new->props.arry_ix = f->prev->len;
                break;
            case jsonst_obj_key:
                p_new->props.obj_key.str = f->str.buf;
                p_new->props.obj_key.str_len = f->str.len;
                break;
        }

        if (p_last == NULL) {
            p_last = p_new;
            p_first = p_new;
        } else {
            p_new->next = p_first;
            p_first->prev = p_new;
            p_first = p_new;
        }
    }

    j->cb(v, p_first, p_last);
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

static void push(_jsonst *j, const jsonst_type /* or jsonst_internal_states */ type) {
    visualize_stack(j->sp);
    printf("push('%c')\n", type);
    j->sp = new_frame(&(j->sp->a), j->sp, type);
    if (j->sp == NULL) {
        crash("OOM");
    }
}

static void pop(_jsonst *j, const jsonst_type /* or jsonst_internal_states */ expect_type) {
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

static void expect_start_value(_jsonst *j, const char c) {
    switch (c) {
        case '[':
            push(j, jsonst_arry);
            emit(j, jsonst_arry);
            return;
        case '{':
            push(j, jsonst_obj);
            emit(j, jsonst_obj);
            return;
        case 'n':
            push(j, jsonst_null);
            j->sp->len++;
            return;
        case 't':
            push(j, jsonst_true);
            j->sp->len++;
            return;
        case 'f':
            push(j, jsonst_false);
            j->sp->len++;
            return;
        case '"':
            push(j, jsonst_str);
            j->sp->str = new_s8(&j->sp->a, STR_ALLOC);
            if (j->sp->str.buf == NULL) {
                crash("OOM");
            }
            return;
        default:
            if (is_digit(c) || c == '-') {
                push(j, jsonst_num);
                j->sp->str = new_s8(&j->sp->a, NUM_STR_ALLOC);
                if (j->sp->str.buf == NULL) {
                    crash("OOM");
                }
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

static void feed(_jsonst *j, const char c) {
    visualize_stack(j->sp);
    printf("feed(\"%c\" = %d)\n", c, c);

    // EOF, with special treatment for numbers (which have no delimiters themselves).
    if (c == EOF && j->sp->type != jsonst_num) {
        if (j->sp->type == jsonst_doc) {
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
        case jsonst_doc:
            if (is_ws(c)) {
                return;
            }
            expect_start_value(j, c);
            return;
        case jsonst_arry:
            if (is_ws(c)) {
                return;
            }
            if (c == ']') {
                emit(j, jsonst_arry_end);
                pop(j, jsonst_arry);
                return;
            }
            push(j, jsonst_arry_elm);
            expect_start_value(j, c);
            return;
        case jsonst_arry_elm:
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j, jsonst_arry_elm);
                assert(j->sp->type == jsonst_arry);
                j->sp->len++;
                push(j, (int)jsonst_arry_elm_next);
                return;
            }
            if (c == ']') {
                pop(j, jsonst_arry_elm);
                emit(j, jsonst_arry_end);
                pop(j, jsonst_arry);
                return;
            }
            crash("invalid char '%c', expected ',' or ']'", c);
            return;
        case jsonst_arry_elm_next:
            if (is_ws(c)) {
                return;
            }
            pop(j, (int)jsonst_arry_elm_next);
            assert(j->sp->type == jsonst_arry);
            push(j, jsonst_arry_elm);
            expect_start_value(j, c);
            return;
        case jsonst_obj:
            if (is_ws(c)) {
                return;
            }
            if (c == '}') {
                emit(j, jsonst_obj_end);
                pop(j, jsonst_obj);
                return;
            }
            if (c == '"') {
                push(j, jsonst_obj_key);
                j->sp->str = new_s8(&j->sp->a, STR_ALLOC);
                if (j->sp->str.buf == NULL) {
                    crash("OOM");
                }
                return;
            }
            crash("invalid char '%c', expected '\"' or '}'", c);
            return;
        case jsonst_obj_post_name:
            if (is_ws(c)) {
                return;
            }
            if (c == ':') {
                pop(j, (int)jsonst_obj_post_name);
                assert(j->sp->type == jsonst_obj_key);
                push(j, (int)jsonst_obj_post_sep);
                // We now still have the key on the stack one below.
                return;
            }
            crash("invalid char '%c': expected ':'", c);
            return;
        case jsonst_obj_post_sep:
            if (is_ws(c)) {
                return;
            }
            pop(j, (int)jsonst_obj_post_sep);
            assert(j->sp->type == jsonst_obj_key);
            push(j, (int)jsonst_obj_next);
            expect_start_value(j, c);
            return;
        case jsonst_obj_next:
            if (is_ws(c)) {
                return;
            }
            if (c == ',') {
                pop(j, (int)jsonst_obj_next);
                pop(j, jsonst_obj_key);
                assert(j->sp->type == jsonst_obj);
                return;
            }
            if (c == '}') {
                pop(j, (int)jsonst_obj_next);
                pop(j, jsonst_obj_key);
                emit(j, jsonst_obj_end);
                pop(j, jsonst_obj);
                return;
            }
            crash("invalid char '%c', expected ',' or '}'", c);
            return;
        case jsonst_null:
            if (c != STR_NULL[j->sp->len]) {
                crash("invalid char '%c', expected 'null' literal", c);
            }
            j->sp->len++;
            if (j->sp->len == STR_NULL_LEN) {
                emit(j, jsonst_null);
                pop(j, jsonst_null);
            }
            return;
        case jsonst_true:
            if (c != STR_TRUE[j->sp->len]) {
                crash("invalid char '%c', expected 'true' literal", c);
            }
            j->sp->len++;
            if (j->sp->len == STR_TRUE_LEN) {
                emit(j, jsonst_true);
                pop(j, jsonst_true);
            }
            return;
        case jsonst_false:
            if (c != STR_FALSE[j->sp->len]) {
                crash("invalid char '%c', expected 'false' literal", c);
            }
            j->sp->len++;
            if (j->sp->len == STR_FALSE_LEN) {
                emit(j, jsonst_false);
                pop(j, jsonst_false);
            }
            return;
        case jsonst_str:
        case jsonst_obj_key:
            if (c == '"') {
                if (j->sp->type == jsonst_str) {
                    emit(j, 0);
                    pop(j, jsonst_str);
                    return;
                }
                assert(j->sp->type == jsonst_obj_key);
                emit(j, jsonst_obj_key);
                push(j, (int)jsonst_obj_post_name);
                // We leave the key on the stack for now.
                return;
            }
            if (c == '\\') {
                push(j, (int)jsonst_str_escp);
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
                push(j, (int)jsonst_str_utf8);
                j->sp->len = 1;  // Remaining bytes for this codepoint.
                return;
            }
            // 3 bytes, 1110xxxx.
            if (((unsigned char)c & 0xF0) == 0xE0) {
                frame_buf_putc(j->sp, c);
                push(j, (int)jsonst_str_utf8);
                j->sp->len = 2;  // Remaining bytes for this codepoint.
                return;
            }
            // 4 bytes, 11110xxx.
            if (((unsigned char)c & 0xF8) == 0xF0) {
                frame_buf_putc(j->sp, c);
                push(j, (int)jsonst_str_utf8);
                j->sp->len = 3;  // Remaining bytes for this codepoint.
                return;
            }
            crash("unhandled string char 0x%x", c);
            return;
        case jsonst_str_escp:
            if (c == 'u') {
                push(j, (int)jsonst_str_escp_uhex);
                j->sp->str = new_s8(&j->sp->a, 4);
                if (j->sp->str.buf == NULL) {
                    crash("OOM");
                }
                return;
            }
            const char unquoted = is_quotable(c);
            if (unquoted != 0) {
                // Write to underlying string buffer.
                assert(j->sp->prev->type == jsonst_str || j->sp->prev->type == jsonst_obj_key);
                frame_buf_putc(j->sp->prev, unquoted);
                pop(j, (int)jsonst_str_escp);
                return;
            }
            crash("invalid quoted char %c 0x%x", c, c);
            return;
        case jsonst_str_utf8:
            // Check that conforms to 10xxxxxx.
            if (((unsigned char)c & 0xC0) != 0x80) {
                crash("invalid utf8-encoding, expected 0b10xxxxxx but got 0x%x", c);
            }
            // Write to underlying string buffer.
            assert(j->sp->prev->type == jsonst_str || j->sp->prev->type == jsonst_obj_key);
            frame_buf_putc(j->sp->prev, c);

            j->sp->len--;
            if (j->sp->len == 0) {
                pop(j, (int)jsonst_str_utf8);
            }
            return;
        case jsonst_str_escp_uhex:
            assert(j->sp->prev->type == jsonst_str_escp);
            assert(j->sp->prev->prev->type == jsonst_str ||
                   j->sp->prev->prev->type == jsonst_obj_key);
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
                push(j, (int)jsonst_str_escp_uhex_utf16);
                j->sp->str = new_s8(&j->sp->a, 4);
                if (j->sp->str.buf == NULL) {
                    crash("OOM");
                }
                j->sp->len = -2;  // Eat the \u sequence
            } else {
                utf8_encode(j->sp->prev->prev, n);
                pop(j, (int)jsonst_str_escp_uhex);
                pop(j, (int)jsonst_str_escp);
            }
            return;
        case jsonst_str_escp_uhex_utf16:
            assert(j->sp->prev->type == jsonst_str_escp_uhex);
            assert(j->sp->prev->prev->type == jsonst_str_escp);
            assert(j->sp->prev->prev->prev->type == jsonst_str ||
                   j->sp->prev->prev->prev->type == jsonst_obj_key);

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
                pop(j, (int)jsonst_str_escp_uhex_utf16);
                pop(j, (int)jsonst_str_escp_uhex);
                pop(j, (int)jsonst_str_escp);
                return;
            }
        case jsonst_num:
            if (!is_num(c)) {
                emit(j, jsonst_num);
                pop(j, jsonst_num);

                // Numbers are the only type delimited by a char which already belongs to the next
                // token. Thus, we have feed that token to the parser again.
                feed(j, c);

                return;
            }
            // One day, maybe do proper parsing instead of leaving the work to strtof.
            frame_buf_putc(j->sp, c);
            return;
        default:
            crash("feed(): unhandled type '%c' = %d", j->sp->type, j->sp->type);
    }
}

void jsonst_feed(jsonst j, const char c) { feed(j, c); }
