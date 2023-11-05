#include "jsonst.h"

#include <assert.h>
#include <stdlib.h>  // strtod()

#include "jsonst_impl.h"
#include "util.h"

static frame *new_frame(arena *a, frame *prev,
                        const jsonst_type /* or jsonst_internal_state */ type) {
    // Allocate the frame itself on the parent arena.
    frame *f = new (a, frame, 1);
    if (f == NULL) {
        return NULL;
    }

    f->prev = prev;
    f->type = type;
    // We now make a copy.
    f->a = *a;

    return f;
}

static _jsonst *new__jsonst(uint8_t *mem, const ptrdiff_t memsz, const jsonst_value_cb cb) {
    arena a = new_arena(mem, memsz);
    jsonst j = new (&a, _jsonst, 1);
    if (j == NULL) {
        return NULL;
    }

    assert(cb != NULL);
    j->cb = cb;

    j->failed = jsonst_success;

    j->sp = new_frame(&a, NULL, jsonst_doc);
    if (j->sp == NULL) {
        return NULL;
    }

    return j;
}

jsonst new_jsonst(uint8_t *mem, const ptrdiff_t memsz, const jsonst_value_cb cb) {
    return new__jsonst(mem, memsz, cb);
}

static jsonst_error frame_buf_putc(frame *f, const char c) __attribute((warn_unused_result));
static jsonst_error frame_buf_putc(frame *f, const char c) {
    assert(f->str.buf != NULL);
    if (f->len >= f->str.len) {
        return jsonst_err_str_buffer_full;
    }
    f->str.buf[f->len] = c;
    f->len++;
    return jsonst_success;
}

static jsonst_error emit(const _jsonst *j, const jsonst_type /* or jsonst_internal_state */ type)
    __attribute((warn_unused_result));
static jsonst_error emit(const _jsonst *j, const jsonst_type /* or jsonst_internal_state */ type) {
    arena scratch = j->sp->a;

    jsonst_value *v = new (&scratch, jsonst_value, 1);
    RET_OOM_IFNULL(v);
    v->type = type;
    switch (type) {
        case jsonst_num: {
            assert(j->sp->type == jsonst_num);
            char *endptr;
            v->val_num = strtod(j->sp->str.buf, &endptr);
            if (endptr != j->sp->str.buf + j->sp->len) {
                return jsonst_err_invalid_number;
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

    // Fill in path, in reverse order of stack.
    jsonst_path *p = NULL;
    for (const frame *f = j->sp; f != NULL; f = f->prev) {
        switch (f->type) {
            case jsonst_arry_elm:
            case jsonst_obj_key:
                break;
            default:
                continue;
        }

        jsonst_path *p_new = new (&scratch, jsonst_path, 1);
        if (p_new == NULL) {
            return jsonst_err_oom;
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

        if (p == NULL) {
            p = p_new;
        } else {
            p_new->next = p;
            p = p_new;
        }
    }

    j->cb(v, p);
    return jsonst_success;
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

static jsonst_error utf8_encode(frame *f, const uint32_t c) __attribute((warn_unused_result));
static jsonst_error utf8_encode(frame *f, const uint32_t c) {
    if (c <= 0x7F) {
        RET_ON_ERR(frame_buf_putc(f, c));
        return jsonst_success;
    } else if (c <= 0x07FF) {
        RET_ON_ERR(frame_buf_putc(f, (((c >> 6) & 0x1F) | 0xC0)));
        RET_ON_ERR(frame_buf_putc(f, (((c >> 0) & 0x3F) | 0x80)));
        return jsonst_success;
    } else if (c <= 0xFFFF) {
        RET_ON_ERR(frame_buf_putc(f, (((c >> 12) & 0x0F) | 0xE0)));
        RET_ON_ERR(frame_buf_putc(f, (((c >> 6) & 0x3F) | 0x80)));
        RET_ON_ERR(frame_buf_putc(f, (((c >> 0) & 0x3F) | 0x80)));
        return jsonst_success;
    } else if (c <= 0x10FFFF) {
        RET_ON_ERR(frame_buf_putc(f, (((c >> 18) & 0x07) | 0xF0)));
        RET_ON_ERR(frame_buf_putc(f, (((c >> 12) & 0x3F) | 0x80)));
        RET_ON_ERR(frame_buf_putc(f, (((c >> 6) & 0x3F) | 0x80)));
        RET_ON_ERR(frame_buf_putc(f, (((c >> 0) & 0x3F) | 0x80)));
        return jsonst_success;
    }
    return jsonst_err_invalid_unicode_codepoint;
}

static jsonst_error push(_jsonst *j, const jsonst_type /* or jsonst_internal_state */ type)
    __attribute((warn_unused_result));
static jsonst_error push(_jsonst *j, const jsonst_type /* or jsonst_internal_state */ type) {
    j->sp = new_frame(&(j->sp->a), j->sp, type);
    if (j->sp == NULL) {
        return jsonst_err_oom;
    }
    return jsonst_success;
}

static void pop(_jsonst *j, const jsonst_type /* or jsonst_internal_state */ expect_type) {
    assert(j->sp != NULL);
    assert(j->sp->type == (int)expect_type);

    j->sp = j->sp->prev;

    if (j->sp == NULL) {
        // Reached EOD.
        return;
    }
}

static jsonst_error expect_start_value(_jsonst *j, const char c) __attribute((warn_unused_result));
static jsonst_error expect_start_value(_jsonst *j, const char c) {
    switch (c) {
        case '[':
            RET_ON_ERR(push(j, jsonst_arry));
            RET_ON_ERR(emit(j, jsonst_arry));
            return jsonst_success;
        case '{':
            RET_ON_ERR(push(j, jsonst_obj));
            RET_ON_ERR(emit(j, jsonst_obj));
            return jsonst_success;
        case 'n':
            RET_ON_ERR(push(j, jsonst_null));
            j->sp->len++;
            return jsonst_success;
        case 't':
            RET_ON_ERR(push(j, jsonst_true));
            j->sp->len++;
            return jsonst_success;
        case 'f':
            RET_ON_ERR(push(j, jsonst_false));
            j->sp->len++;
            return jsonst_success;
        case '"':
            RET_ON_ERR(push(j, jsonst_str));
            j->sp->str = new_s8(&j->sp->a, STR_ALLOC);
            RET_OOM_IFNULL(j->sp->str.buf);
            return jsonst_success;
        default:
            if (is_digit(c) || c == '-') {
                RET_ON_ERR(push(j, jsonst_num));
                j->sp->str = new_s8(&j->sp->a, NUM_STR_ALLOC);
                RET_OOM_IFNULL(j->sp->str.buf);
                RET_ON_ERR(frame_buf_putc(j->sp, c));
                return jsonst_success;
            }
            return jsonst_err_expected_new_value;
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

static jsonst_error feed(_jsonst *j, const char c) __attribute((warn_unused_result));
static jsonst_error feed(_jsonst *j, const char c) {
    // EOF, with special treatment for numbers (which have no delimiters themselves).
    if (c == JSONST_EOF && j->sp->type != jsonst_num) {
        if (j->sp->type == jsonst_doc) {
            j->sp = NULL;
        }
        assert(j->sp == NULL);
        return jsonst_success;
    }

    if (j->sp == NULL) {
        if (is_ws(c)) {
            return jsonst_success;
        }
        return jsonst_err_end_of_doc;
    }

    switch (j->sp->type) {
        case jsonst_doc:
            if (is_ws(c)) {
                return jsonst_success;
            }
            RET_ON_ERR(expect_start_value(j, c));
            return jsonst_success;
        case jsonst_arry:
            if (is_ws(c)) {
                return jsonst_success;
            }
            if (c == ']') {
                RET_ON_ERR(emit(j, jsonst_arry_end));
                pop(j, jsonst_arry);
                return jsonst_success;
            }
            RET_ON_ERR(push(j, jsonst_arry_elm));
            RET_ON_ERR(expect_start_value(j, c));
            return jsonst_success;
        case jsonst_arry_elm:
            if (is_ws(c)) {
                return jsonst_success;
            }
            if (c == ',') {
                pop(j, jsonst_arry_elm);
                assert(j->sp->type == jsonst_arry);
                j->sp->len++;
                RET_ON_ERR(push(j, (int)jsonst_arry_elm_next));
                return jsonst_success;
            }
            if (c == ']') {
                pop(j, jsonst_arry_elm);
                RET_ON_ERR(emit(j, jsonst_arry_end));
                pop(j, jsonst_arry);
                return jsonst_success;
            }
            // Expected ',' or ']'.
            return jsonst_err_unexpected_char;
        case jsonst_arry_elm_next:
            if (is_ws(c)) {
                return jsonst_success;
            }
            pop(j, (int)jsonst_arry_elm_next);
            assert(j->sp->type == jsonst_arry);
            RET_ON_ERR(push(j, jsonst_arry_elm));
            RET_ON_ERR(expect_start_value(j, c));
            return jsonst_success;
        case jsonst_obj:
            if (is_ws(c)) {
                return jsonst_success;
            }
            if (c == '}') {
                RET_ON_ERR(emit(j, jsonst_obj_end));
                pop(j, jsonst_obj);
                return jsonst_success;
            }
            if (c == '"') {
                RET_ON_ERR(push(j, jsonst_obj_key));
                j->sp->str = new_s8(&j->sp->a, STR_ALLOC);
                RET_OOM_IFNULL(j->sp->str.buf);
                return jsonst_success;
            }
            // Expected '\"' or '}'.
            return jsonst_err_unexpected_char;
        case jsonst_obj_post_name:
            if (is_ws(c)) {
                return jsonst_success;
            }
            if (c == ':') {
                pop(j, (int)jsonst_obj_post_name);
                assert(j->sp->type == jsonst_obj_key);
                RET_ON_ERR(push(j, (int)jsonst_obj_post_sep));
                // We now still have the key on the stack one below.
                return jsonst_success;
            }
            // Expected ':'.
            return jsonst_err_unexpected_char;
        case jsonst_obj_post_sep:
            if (is_ws(c)) {
                return jsonst_success;
            }
            pop(j, (int)jsonst_obj_post_sep);
            assert(j->sp->type == jsonst_obj_key);
            RET_ON_ERR(push(j, (int)jsonst_obj_next));
            RET_ON_ERR(expect_start_value(j, c));
            return jsonst_success;
        case jsonst_obj_next:
            if (is_ws(c)) {
                return jsonst_success;
            }
            if (c == ',') {
                pop(j, (int)jsonst_obj_next);
                pop(j, jsonst_obj_key);
                assert(j->sp->type == jsonst_obj);
                return jsonst_success;
            }
            if (c == '}') {
                pop(j, (int)jsonst_obj_next);
                pop(j, jsonst_obj_key);
                RET_ON_ERR(emit(j, jsonst_obj_end));
                pop(j, jsonst_obj);
                return jsonst_success;
            }
            // Expected ',' or '}'.
            return jsonst_err_unexpected_char;
        case jsonst_null:
            if (c != STR_NULL[j->sp->len]) {
                return jsonst_err_invalid_literal;
            }
            j->sp->len++;
            if (j->sp->len == STR_NULL_LEN) {
                RET_ON_ERR(emit(j, jsonst_null));
                pop(j, jsonst_null);
            }
            return jsonst_success;
        case jsonst_true:
            if (c != STR_TRUE[j->sp->len]) {
                return jsonst_err_invalid_literal;
            }
            j->sp->len++;
            if (j->sp->len == STR_TRUE_LEN) {
                RET_ON_ERR(emit(j, jsonst_true));
                pop(j, jsonst_true);
            }
            return jsonst_success;
        case jsonst_false:
            if (c != STR_FALSE[j->sp->len]) {
                return jsonst_err_invalid_literal;
            }
            j->sp->len++;
            if (j->sp->len == STR_FALSE_LEN) {
                RET_ON_ERR(emit(j, jsonst_false));
                pop(j, jsonst_false);
            }
            return jsonst_success;
        case jsonst_str:
        case jsonst_obj_key:
            if (c == '"') {
                if (j->sp->type == jsonst_str) {
                    RET_ON_ERR(emit(j, 0));
                    pop(j, jsonst_str);
                    return jsonst_success;
                }
                assert(j->sp->type == jsonst_obj_key);
                RET_ON_ERR(emit(j, jsonst_obj_key));
                RET_ON_ERR(push(j, (int)jsonst_obj_post_name));
                // We leave the key on the stack for now.
                return jsonst_success;
            }
            if (c == '\\') {
                RET_ON_ERR(push(j, (int)jsonst_str_escp));
                return jsonst_success;
            }
            // Single byte, 0xxxxxxx.
            if (((unsigned char)c & 0x80) == 0) {
                if (is_cntrl(c)) {
                    return jsonst_err_invalid_control_char;
                }
                RET_ON_ERR(frame_buf_putc(j->sp, c));
                return jsonst_success;
            }
            // 2 bytes, 110xxxxx.
            if (((unsigned char)c & 0xE0) == 0xC0) {
                RET_ON_ERR(frame_buf_putc(j->sp, c));
                RET_ON_ERR(push(j, (int)jsonst_str_utf8));
                j->sp->len = 1;  // Remaining bytes for this codepoint.
                return jsonst_success;
            }
            // 3 bytes, 1110xxxx.
            if (((unsigned char)c & 0xF0) == 0xE0) {
                RET_ON_ERR(frame_buf_putc(j->sp, c));
                RET_ON_ERR(push(j, (int)jsonst_str_utf8));
                j->sp->len = 2;  // Remaining bytes for this codepoint.
                return jsonst_success;
            }
            // 4 bytes, 11110xxx.
            if (((unsigned char)c & 0xF8) == 0xF0) {
                RET_ON_ERR(frame_buf_putc(j->sp, c));
                RET_ON_ERR(push(j, (int)jsonst_str_utf8));
                j->sp->len = 3;  // Remaining bytes for this codepoint.
                return jsonst_success;
            }
            return jsonst_err_invalid_utf8_encoding;
        case jsonst_str_escp:
            if (c == 'u') {
                RET_ON_ERR(push(j, (int)jsonst_str_escp_uhex));
                j->sp->str = new_s8(&j->sp->a, 4);
                RET_OOM_IFNULL(j->sp->str.buf);
                return jsonst_success;
            }
            const char unquoted = is_quotable(c);
            if (unquoted != 0) {
                // Write to underlying string buffer.
                assert(j->sp->prev->type == jsonst_str || j->sp->prev->type == jsonst_obj_key);
                RET_ON_ERR(frame_buf_putc(j->sp->prev, unquoted));
                pop(j, (int)jsonst_str_escp);
                return jsonst_success;
            }
            return jsonst_err_invalid_quoted_char;
        case jsonst_str_utf8:
            // Check that conforms to 10xxxxxx.
            if (((unsigned char)c & 0xC0) != 0x80) {
                return jsonst_err_invalid_utf8_encoding;
            }
            // Write to underlying string buffer.
            assert(j->sp->prev->type == jsonst_str || j->sp->prev->type == jsonst_obj_key);
            RET_ON_ERR(frame_buf_putc(j->sp->prev, c));

            j->sp->len--;
            if (j->sp->len == 0) {
                pop(j, (int)jsonst_str_utf8);
            }
            return jsonst_success;
        case jsonst_str_escp_uhex:
            assert(j->sp->prev->type == jsonst_str_escp);
            assert(j->sp->prev->prev->type == jsonst_str ||
                   j->sp->prev->prev->type == jsonst_obj_key);
            if (!is_xdigit(c)) {
                return jsonst_err_invalid_hex_digit;
            }
            RET_ON_ERR(frame_buf_putc(j->sp, c));
            if (j->sp->len < 4) {
                return jsonst_success;
            }

            const uint32_t n = parse_hex4(j->sp->str);
            if (is_utf16_high_surrogate(n)) {
                RET_ON_ERR(push(j, (int)jsonst_str_escp_uhex_utf16));
                j->sp->str = new_s8(&j->sp->a, 4);
                RET_OOM_IFNULL(j->sp->str.buf);
                j->sp->len = -2;  // Eat the \u sequence
            } else {
                RET_ON_ERR(utf8_encode(j->sp->prev->prev, n));
                pop(j, (int)jsonst_str_escp_uhex);
                pop(j, (int)jsonst_str_escp);
            }
            return jsonst_success;
        case jsonst_str_escp_uhex_utf16:
            assert(j->sp->prev->type == jsonst_str_escp_uhex);
            assert(j->sp->prev->prev->type == jsonst_str_escp);
            assert(j->sp->prev->prev->prev->type == jsonst_str ||
                   j->sp->prev->prev->prev->type == jsonst_obj_key);

            if (j->sp->len == -2 && c == '\\') {
                j->sp->len++;
                return jsonst_success;
            }
            if (j->sp->len == -1 && c == 'u') {
                j->sp->len++;
                return jsonst_success;
            }
            if (j->sp->len < 0) {
                return jsonst_err_invalid_utf16_surrogate;
            }

            if (!is_xdigit(c)) {
                return jsonst_err_invalid_hex_digit;
            }
            RET_ON_ERR(frame_buf_putc(j->sp, c));
            if (j->sp->len < 4) {
                return jsonst_success;
            }

            {
                const uint32_t low = parse_hex4(j->sp->str);
                if (!is_utf16_low_surrogate(low)) {
                    return jsonst_err_invalid_utf16_surrogate;
                }
                const uint32_t high = parse_hex4(j->sp->prev->str);
                assert(is_utf16_high_surrogate(high));

                const uint32_t decoded = ((high - 0xd800) << 10 | (low - 0xdc00)) + 0x10000;

                RET_ON_ERR(utf8_encode(j->sp->prev->prev->prev, decoded));
                pop(j, (int)jsonst_str_escp_uhex_utf16);
                pop(j, (int)jsonst_str_escp_uhex);
                pop(j, (int)jsonst_str_escp);
                return jsonst_success;
            }
        case jsonst_num:
            if (!is_num(c)) {
                RET_ON_ERR(emit(j, jsonst_num));
                pop(j, jsonst_num);

                // Numbers are the only type delimited by a char which already belongs to the next
                // token. Thus, we have feed that token to the parser again.
                RET_ON_ERR(feed(j, c));

                return jsonst_success;
            }
            // One day, maybe do proper parsing instead of leaving all the work to strtof.
            RET_ON_ERR(frame_buf_putc(j->sp, c));
            return jsonst_success;
        default:
            assert(false);
            return jsonst_err_internal_bug;
    }
}

jsonst_error jsonst_feed(jsonst j, const char c) {
    if (j->failed != jsonst_success) {
        return jsonst_err_previous_error;
    }

    const jsonst_error ret = feed(j, c);
    if (ret != jsonst_success) {
        j->failed = ret;
    }
    return ret;
}

jsonst_error jsonst_feed_doc(jsonst j, const char *doc, const ptrdiff_t docsz) {
    for (ptrdiff_t i = 0; i < docsz; i++) {
        const jsonst_error ret = jsonst_feed(j, doc[i]);
        if (ret != jsonst_success) {
            return ret;
        }
    }
    return jsonst_feed(j, JSONST_EOF);
}
