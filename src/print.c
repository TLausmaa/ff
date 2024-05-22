#include <stdio.h>

#include "main.h"

void print_results(struct results_t* results) {
    for (int i = 0; i < results->p_count; i++) {
        printf("%s\n", results->partial[i]);
    }

    if (results->e_count > 0) {
        printf("----------------------------------------\n");
    }

    for (int i = 0; i < results->e_count; i++) {
        printf("%s\n", results->exact[i]);
    }

    // printf("File types, c: %d, ts: %d, js: %d\n", results.stats->num_c, results.stats->num_ts, results.stats->num_js);
}

void print_help(int argc, char** argv) {
    fprintf(stderr, "Usage: %s [query]\n", argv[0]);
    fprintf(stderr, "       %s [path] [query]\n", argv[0]);
    fprintf(stderr, "    Options:\n");
    fprintf(stderr, "       [-e pattern]\n");
    fprintf(stderr, "           Ignore files specified in pattern, where pattern is a comma-separated list of file name endings.\n");
    fprintf(stderr, "           EXAMPLE: \"-e .js,.map\" would ignore any file ending with either '.js', or '.map'.\n");
}
