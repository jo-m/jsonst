#include "jsonst.h"

#include <assert.h>
#include <stddef.h>
#include <stdint.h>

#define Sizeof(x) (ptrdiff_t)sizeof(x)
#define Alignof(x) (ptrdiff_t) _Alignof(x)
#define Countof(a) (Sizeof(a) / Sizeof(*(a)))
#define Lengthof(s) (Countof(s) - 1)

// Utils - arena alloc, strings.

// Inspired by https://nullprogram.com/blog/2023/09/27/.
typedef struct {
    uint8_t *beg;
    uint8_t *end;
} arena;

// Inspired by https://nullprogram.com/blog/2023/10/08/, modified to always be null byte terminated.
// The length field is excluding the 0 byte.
typedef struct {
    char *buf;
    ptrdiff_t len;
} s8;

#define s8(s) \
    (s8) { (char *)s, lengthof(s) }

static arena new_arena(uint8_t *mem, const ptrdiff_t memsz) {
    arena a = {0};
    a.beg = mem;
    a.end = a.beg ? a.beg + memsz : 0;
    return a;
}

// Returns NULL on OOM.
static __attribute((malloc, alloc_size(2, 4), alloc_align(3))) void *alloc(arena *a, ptrdiff_t size,
                                                                           ptrdiff_t align,
                                                                           ptrdiff_t count) {
    ptrdiff_t avail = a->end - a->beg;
    ptrdiff_t padding = -(uintptr_t)a->beg & (align - 1);
    if (count > (avail - padding) / size) {
        return NULL;
    }
    ptrdiff_t total = size * count;
    uint8_t *p = a->beg + padding;
    a->beg += padding + total;

    // memset(p, 0, total)
    for (ptrdiff_t i = 0; i < total; i++) {
        p[i] = 0;
    }

#ifndef NDEBUG
    for (uint8_t *m = a->beg; m <= a->end; m++) {
        *m = '~';
    }
#endif

    return p;
}

#define New(a, t, n) (t *)alloc(a, Sizeof(t), _Alignof(t), n)

// May return s8.buf = NULL on OOM.
static s8 new_s8(arena *a, const ptrdiff_t len) {
    s8 s = {0};
    // +1 is for C 0 byte interop.
    s.buf = New(a, char, len + 1);
    if (s.buf != NULL) {
        s.len = len;
    }
    return s;
}

// Implementation structs.

// In most places where jsonst_type is used, jsonst_internal_state may also be used.
// Thus, we need to be careful to no accidentally introduce overlaps.
typedef enum {
    jsonst_arry_elm_next = '+',

    jsonst_obj_post_key = ':',
    jsonst_obj_post_sep = '-',
    jsonst_obj_next = ';',

    jsonst_str_utf8 = '8',
    jsonst_str_escp = '\\',
    jsonst_str_escp_uhex = '%',
    jsonst_str_escp_uhex_utf16 = '6',
} jsonst_internal_state;

typedef struct frame frame;
struct frame {
    frame *prev;
    int type;  // Is a jsonst_type or jsonst_internal_state.
    arena a;

    // Processing state, usage depends on type.
    // Some types will use len separately without str.
    s8 str;
    ptrdiff_t len;
};

typedef struct _jsonst _jsonst;
typedef struct _jsonst {
    jsonst_value_cb cb;
    void *cb_user_data;
    jsonst_config config;
    jsonst_error failed;
    frame *sp;
} _jsonst;

// Helpers for error handling.
#define RET_ON_ERR(expr)                 \
    {                                    \
        const jsonst_error ret = (expr); \
        if (ret != jsonst_success) {     \
            return ret;                  \
        }                                \
    }

#define RET_OOM_IFNULL(expr)   \
    if ((expr) == NULL) {      \
        return jsonst_err_oom; \
    }

// Implementation.

static frame *new_frame(arena a, frame *prev,
                        const jsonst_type /* or jsonst_internal_state */ type) {
    // Allocate the frame on its own copy of the arena.
    frame *f = New(&a, frame, 1);
    if (f == NULL) {
        return NULL;
    }

    f->prev = prev;
    f->type = type;
    f->a = a;

    return f;
}

static _jsonst *new__jsonst(uint8_t *mem, const ptrdiff_t memsz, const jsonst_value_cb cb,
                            void *cb_user_data, const jsonst_config conf) {
    arena a = new_arena(mem, memsz);
    jsonst j = New(&a, _jsonst, 1);
    if (j == NULL) {
        return NULL;
    }

    assert(cb != NULL);
    j->cb = cb;
    j->cb_user_data = cb_user_data;

    j->config = conf;
    if (j->config.str_alloc_bytes == 0) {
        j->config.str_alloc_bytes = JSONST_DEFAULT_STR_ALLOC_BYTES;
    }
    if (j->config.obj_key_alloc_bytes == 0) {
        j->config.obj_key_alloc_bytes = JSONST_DEFAULT_OBJ_KEY_ALLOC_BYTES;
    }
    if (j->config.num_alloc_bytes == 0) {
        j->config.num_alloc_bytes = JSONST_DEFAULT_NUM_ALLOC_BYTES;
    }

    j->failed = jsonst_success;

    j->sp = new_frame(a, NULL, jsonst_doc);
    if (j->sp == NULL) {
        return NULL;
    }

    return j;
}

jsonst new_jsonst(uint8_t *mem, const ptrdiff_t memsz, const jsonst_value_cb cb, void *cb_user_data,
                  const jsonst_config conf) {
    return new__jsonst(mem, memsz, cb, cb_user_data, conf);
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

static bool is_digit(const char c);

typedef enum {
    num_init,
    num_int_minus,
    num_int_zero,
    num_int_digit,
    num_int_dot,
    num_int_frac,
    num_exp_e,
    num_exp_sign,
    num_exp_int,
} number_state;

// By not directly integrating this into the parsing state machine we might waste
// a bit of performance, but it's much easier to read.
static bool is_valid_json_number(const char *buf, const ptrdiff_t len) {
    number_state s = num_init;
    for (ptrdiff_t i = 0; i < len; i++) {
        const char c = buf[i];
        switch (s) {
            case num_init:
                if (c == '-') {
                    s = num_int_minus;
                    continue;
                }
                if (c == '0') {
                    s = num_int_zero;
                    continue;
                }
                if (!is_digit(c)) return false;
                s = num_int_digit;
                continue;
            case num_int_minus:
                if (c == '0') {
                    s = num_int_zero;
                    continue;
                }
                if (!is_digit(c)) return false;
                s = num_int_digit;
                continue;
            case num_int_zero:
                if (is_digit(c)) return false;
                if (c == '.') {
                    s = num_int_dot;
                    continue;
                }
                if (c == 'E' || c == 'e') {
                    s = num_exp_e;
                    continue;
                }
                return false;
            case num_int_digit:
                if (is_digit(c)) continue;
                if (c == '.') {
                    s = num_int_dot;
                    continue;
                }
                if (c == 'E' || c == 'e') {
                    s = num_exp_e;
                    continue;
                }
                return false;
            case num_int_dot:
                if (!is_digit(c)) return false;
                s = num_int_frac;
                continue;
            case num_int_frac:
                if (is_digit(c)) continue;
                if (c != 'E' && c != 'e') return false;
                s = num_exp_e;
                continue;
            case num_exp_e:
                if (c == '-' || c == '+') {
                    s = num_exp_sign;
                    continue;
                }
                if (!is_digit(c) || c == '0') return false;
                s = num_exp_int;
                continue;
            case num_exp_sign:
                if (!is_digit(c) || c == '0') return false;
                s = num_exp_int;
                continue;
            case num_exp_int:
                if (!is_digit(c)) return false;
        }
    }

    return s == num_int_zero || s == num_int_digit || s == num_int_frac || s == num_exp_int;
}

static jsonst_error emit(const _jsonst *j, const jsonst_type /* or jsonst_internal_state */ type)
    __attribute((warn_unused_result));
static jsonst_error emit(const _jsonst *j, const jsonst_type /* or jsonst_internal_state */ type) {
    arena scratch = j->sp->a;

    jsonst_value *v = New(&scratch, jsonst_value, 1);
    RET_OOM_IFNULL(v);
    v->type = type;
    switch (type) {
        case jsonst_num: {
            assert(j->sp->type == jsonst_num);
            if (!is_valid_json_number(j->sp->str.buf, j->sp->len)) {
                return jsonst_err_invalid_number;
            }
            v->val_str.str = j->sp->str.buf;
            v->val_str.str_len = j->sp->len;
        } break;
        case jsonst_str:
        case jsonst_obj_key:
            assert(j->sp->type == jsonst_str || j->sp->type == jsonst_obj_key);
            v->val_str.str = j->sp->str.buf;
            v->val_str.str_len = j->sp->len;
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

        jsonst_path *p_new = New(&scratch, jsonst_path, 1);
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
                p_new->props.obj_key.str_len = f->len;
                break;
        }

        if (p == NULL) {
            p = p_new;
        } else {
            p_new->next = p;
            p = p_new;
        }
    }

    j->cb(j->cb_user_data, v, p);
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

static bool is_cntrl(const char c) { return c >= 0x00 && c <= 0x1F; }

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
    j->sp = new_frame(j->sp->a, j->sp, type);
    if (j->sp == NULL) {
        return jsonst_err_oom;
    }
    return jsonst_success;
}

static void pop(_jsonst *j, __attribute((unused))
                            const jsonst_type /* or jsonst_internal_state */ expect_type) {
    assert(j->sp != NULL);
    assert(j->sp->type == (int)expect_type);

    j->sp = j->sp->prev;
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
            j->sp->str = new_s8(&j->sp->a, j->config.str_alloc_bytes);
            RET_OOM_IFNULL(j->sp->str.buf);
            return jsonst_success;
        default:
            if (is_digit(c) || c == '-') {
                RET_ON_ERR(push(j, jsonst_num));
                j->sp->str = new_s8(&j->sp->a, j->config.num_alloc_bytes);
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

#define STR_NULL "null"
#define STR_NULL_LEN (Sizeof(STR_NULL) - 1)
#define STR_TRUE "true"
#define STR_TRUE_LEN (Sizeof(STR_TRUE) - 1)
#define STR_FALSE "false"
#define STR_FALSE_LEN (Sizeof(STR_FALSE) - 1)

static jsonst_error feed(_jsonst *j, const char c) __attribute((warn_unused_result));
static jsonst_error feed(_jsonst *j, const char c) {
    if (j->sp == NULL) {
        if (is_ws(c)) {
            return jsonst_success;
        }
        return jsonst_err_end_of_doc;
    }

    // EOF, with special treatment for numbers (which have no delimiters themselves).
    // TODO: Maybe move to switch statement.
    if (c == JSONST_EOF && j->sp->type != jsonst_num) {
        if (j->sp->type == jsonst_doc && j->sp->len > 0) {
            j->sp = NULL;
        }
        if (j->sp != NULL) {
            return jsonst_err_invalid_eof;
        }
        return jsonst_success;
    }

    switch (j->sp->type) {
        case jsonst_doc:
            if (is_ws(c)) {
                return jsonst_success;
            }
            if (j->sp->len > 0) {
                return jsonst_err_end_of_doc;
            }
            j->sp->len++;
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
                if (j->sp->len > 0) {
                    // We are already after the comma.
                    return jsonst_err_expected_new_key;
                }
                RET_ON_ERR(emit(j, jsonst_obj_end));
                pop(j, jsonst_obj);
                return jsonst_success;
            }
            if (c == '"') {
                RET_ON_ERR(push(j, jsonst_obj_key));
                j->sp->str = new_s8(&j->sp->a, j->config.obj_key_alloc_bytes);
                RET_OOM_IFNULL(j->sp->str.buf);
                return jsonst_success;
            }
            // Expected '\"' or '}'.
            return jsonst_err_unexpected_char;
        case jsonst_obj_post_key:
            if (is_ws(c)) {
                return jsonst_success;
            }
            if (c == ':') {
                pop(j, (int)jsonst_obj_post_key);
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
                j->sp->len++;  // Count members.
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
                    RET_ON_ERR(emit(j, jsonst_str));
                    pop(j, jsonst_str);
                    return jsonst_success;
                }
                assert(j->sp->type == jsonst_obj_key);
                RET_ON_ERR(emit(j, jsonst_obj_key));
                RET_ON_ERR(push(j, (int)jsonst_obj_post_key));
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

jsonst_feed_doc_ret jsonst_feed_doc(jsonst j, const char *doc, const size_t docsz) {
    jsonst_feed_doc_ret ret = {0};

    for (ret.parsed_bytes = 0; ret.parsed_bytes < docsz; ret.parsed_bytes++) {
        ret.err = jsonst_feed(j, doc[ret.parsed_bytes]);
        if (ret.err != jsonst_success) {
            return ret;
        }
    }
    ret.err = jsonst_feed(j, JSONST_EOF);
    return ret;
}
