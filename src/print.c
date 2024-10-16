#include <stdio.h>

#include "main.h"

void print_results(results_t* results) {
    for (int i = 0; i < results->p_count; i++) {
        printf("%s\n", results->partial[i]);
    }

    for (int i = 0; i < results->e_count; i++) {
        printf("%s\n", results->exact[i]);
    }
}

void print_help(int argc, char** argv) {
    fprintf(stderr, "Usage: %s [query]\n", argv[0]);
    fprintf(stderr, "       %s [path] [query]\n", argv[0]);
    fprintf(stderr, "       %s [path] [query] [-option {foo bar}]\n", argv[0]);
    fprintf(stderr, "    Options:\n");
    fprintf(stderr, "       [-i pattern]\n");
    fprintf(stderr, "           Ignore files specified in pattern, where pattern is a whitespace separated list of file name endings.\n");
    fprintf(stderr, "           EXAMPLE: \"-i .js .map\" would ignore any file ending with either '.js', or '.map'.\n");
    fprintf(stderr, "       [-o pattern]\n");
    fprintf(stderr, "           List only files specified in pattern, where pattern is a whitespace separated list of file name endings.\n");
    fprintf(stderr, "           EXAMPLE: \"-o .c .h\" would list only files ending with either '.c', or '.h'.\n");
}
