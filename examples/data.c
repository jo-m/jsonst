// Sample code showing how to parse and process data.
// To run:
//
//   bazel run //examples:data examples/testdata/fluglaerm-iselisberg.json

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/jsonst_cstd.h"

typedef struct {
    double *val;
    ptrdiff_t cap;
    ptrdiff_t len;
} dvec;

static dvec new_dvec(const size_t cap) {
    dvec ret = {0};
    ret.val = calloc(cap, sizeof(double));
    assert(ret.val != NULL);
    ret.len = 0;
    ret.cap = cap;
    return ret;
}

static void dvec_push(dvec *vec, const double val) {
    assert(vec->len < vec->cap);
    vec->val[vec->len] = val;
    vec->len++;
}

static void dvec_free(dvec *vec) { free(vec->val); }

static void cb(void *user_data, const jsonst_value *value, const jsonst_path *path) {
    if (value->type != jsonst_num) {
        return;
    }

    if (path->type != jsonst_arry_elm) {
        return;
    }

    if (path->next == NULL || path->next->type != jsonst_obj_key) {
        return;
    }

    if (strncmp(path->next->props.obj_key.str, "temperatur_degc_max",
                path->next->props.obj_key.str_len) != 0) {
        return;
    }

    char *endptr;
    double num = strtod(value->val_str.str, &endptr);
    assert(endptr == value->val_str.str + value->val_str.str_len);

    dvec *vec = (dvec *)user_data;
    dvec_push(vec, num);
}

int main(const int argc, const char **argv) {
    // Open file.
    if (argc != 2) {
        printf("usage: data <input>.json\n");
        return EXIT_FAILURE;
    }
    FILE *inf = fopen(argv[1], "r");
    if (inf == NULL) {
        fprintf(stderr, "Unable to open file '%s'!\n", argv[1]);
        return EXIT_FAILURE;
    }

    // Prepare results vec.
    dvec vec = new_dvec(276);

    // Prepare parser.
    const ptrdiff_t memsz = 1024 * 1;
    void *mem = malloc(memsz);
    assert(mem != NULL);
    jsonst_config conf = {0};
    jsonst j = new_jsonst(mem, memsz, cb, &vec, conf);

    const jsonst_feed_doc_ret ret = jsonst_feed_fstream(j, inf);
    if (ret.err != jsonst_success) {
        fprintf(stderr, "Parsing error: %d\n", ret.err);

        return EXIT_FAILURE;
    }

    // Print results.
    for (ptrdiff_t i = 0; i < vec.len; i++) {
        printf("[%td] = %f\n", i, vec.val[i]);
    }

    // Cleanup.
    fclose(inf);
    free(mem);
    dvec_free(&vec);
    return EXIT_SUCCESS;
}
