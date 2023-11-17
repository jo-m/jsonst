#pragma once

#include <stdio.h>

#include "jsonst.h"

// Like jsonst_feed_doc(), but works with a stdio FILE stream.
static inline jsonst_feed_doc_ret jsonst_feed_fstream(jsonst j, FILE *f) {
    jsonst_feed_doc_ret ret;
    ret.err = jsonst_success;
    ret.parsed_bytes = 0;

    while (1) {
        int c = fgetc(f);
        if (c == EOF) {
            c = JSONST_EOF;
        }

        ret.err = jsonst_feed(j, (char)c);
        if (ret.err != jsonst_success) {
            return ret;
        }

        if (c == JSONST_EOF) {
            break;
        }

        ret.parsed_bytes++;
    }

    return ret;
}
