#include <string.h>
#include <stdio.h>

#include "main.h"

void count_file_type(char* filename) {
    // char buf[5];
    int len = strlen(filename);
    int idx = -1;
    for (int i = len - 1; i > 0; i--) {
        if (filename[i] == '.') {
            idx = i;
        }
        if (len - 1 - i > 5) {
            break;
        }
    }
    char* p = &filename[idx];
    printf("idx is %d, str is %s\n", idx, p);

    /*
    if (strcmp(".c", ext) == 0) {
        results.stats->num_c++;
        return;
    } else if (strcmp(".ts", ext) == 0) {
        results.stats->num_ts++;
        return;
    } else if (strcmp(".js", ext) == 0) {
        results.stats->num_js++;
    }*/
}


