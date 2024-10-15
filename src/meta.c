#include <string.h>
#include <stdio.h>

#include "main.h"

void count_file_type(char* filename, results_t* results) {
    int len = strlen(filename);
    int idx = -1;
    for (int i = len - 1; i > 0; i--) {
        if (filename[i] == '.') {
            idx = i;
        }
        if (len - 1 - i > 8) {
            break;
        }
    }
    char* p = &filename[idx];

    if (strcmp(".ts", p) == 0) {
        results->stats->num_ts++;
    }
}
