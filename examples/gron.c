// This example binary implements a simplified version of gron (https://github.com/tomnomnom/gron).
// Limitations:
// - Limited CLI arguments parsing.
// - Does not escape strings on output.
// - Can only parse, not write, JSON.
//
// To run with the example JSON file:
//
//   bazel run //examples:gron examples/testdata/docker-inspect.json
//
// or
//
//   bazel run //examples:gron -- - < examples/testdata/docker-inspect.json

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "src/jsonst.h"
#include "src/jsonst_util.h"

static void jsonst_path_print(const jsonst_path *path) {
    printf("json");
    for (const jsonst_path *p = path; p != NULL; p = p->next) {
        switch (p->type) {
            case jsonst_arry_elm:
                printf("[%d]", p->props.arry_ix);
                break;
            case jsonst_obj_key:
                printf(".%.*s", (int)p->props.obj_key.str_len, p->props.obj_key.str);
                break;
            default:
                break;
        }
    }
}

static void cb(__attribute((unused)) void *cb_user_data, const jsonst_value *value,
               const jsonst_path *p) {
    switch (value->type) {
        case jsonst_obj_key:
        case jsonst_arry_end:
        case jsonst_obj_end:
            return;
        default:
            break;
    }

    jsonst_path_print(p);
    printf(" = ");

    switch (value->type) {
        case jsonst_null:
            printf("null");
            break;
        case jsonst_true:
            printf("true");
            break;
        case jsonst_false:
            printf("true");
            break;
        case jsonst_num:
            printf("%f", value->val_num);
            break;
        case jsonst_str:
            printf("\"%.*s\"", (int)value->val_str.str_len, value->val_str.str);
            break;
        case jsonst_arry:
            printf("[]");
            break;
        case jsonst_obj:
            printf("{}");
            break;
        default:
            fprintf(stderr, "Unhandled type [%d], this is a bug!\n", (int)value->type);
            exit(1);
            abort();
    }
    printf(";\n");
}

int main(const int argc, const char **argv) {
    // Open file.
    if (argc != 2) {
        printf("usage: gron <input>.json\n");
        return EXIT_FAILURE;
    }
    FILE *inf;
    if (strlen(argv[1]) == 1 && argv[1][0] == '-') {
        inf = stdin;
    } else {
        inf = fopen(argv[1], "r");
        if (inf == NULL) {
            fprintf(stderr, "Unable to open file '%s'!\n", argv[1]);
            return EXIT_FAILURE;
        }
    }

    // Prepare parser.
    const ptrdiff_t memsz = 1024 * 8;
    void *mem = malloc(memsz);
    assert(mem != NULL);
    jsonst_config conf = {0};
    conf.str_alloc_bytes = 2048;
    conf.obj_key_alloc_bytes = 16;
    conf.num_alloc_bytes = 16;
    conf.strtod = strtod;
    jsonst j = new_jsonst(mem, memsz, cb, NULL, conf);

    // Process file.
    while (1) {
        int c = fgetc(inf);
        if (c == EOF) {
            c = JSONST_EOF;
        }

        const jsonst_error ret = jsonst_feed(j, (char)c);
        if (ret != jsonst_success) {
            fprintf(stderr, "Parsing error: %s\n", jsonst_error_to_str(ret));
            return EXIT_FAILURE;
        }

        if (c == JSONST_EOF) {
            break;
        }
    }

    // Cleanup.
    fclose(inf);
    free(mem);
    return EXIT_SUCCESS;
}
