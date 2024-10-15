#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"

int parse_args(int argc, char** argv, exec_args_t* args) {
    if (argc < 2) {
        return 0;
    }

    if (strcmp(argv[1], "help") == 0 || strcmp(argv[1], "--help") == 0 || strcmp(argv[1], "-h") == 0) {
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            if (args->path == NULL) {
                args->path = malloc(strlen(argv[i]) + 1);
                strcpy(args->path, argv[i]);
                continue;
            }
            if (args->query == NULL) {
                args->query = malloc(strlen(argv[i]) + 1);
                strcpy(args->query, argv[i]);
                continue;
            }
        }
        
        if (strcmp(argv[i], "-e") == 0) {
            if (i == argc - 1) {
                printf("Missing parameters for option -e\n");
            }

            int arr_len = 0;
            for (int j = i + 1; j < argc; j++) {
                if (argv[j][0] == '-') {
                    break;
                }
                args->ignored_files = realloc(args->ignored_files, sizeof(char*) * (arr_len + 1));
                args->ignored_files[arr_len] = malloc(strlen(argv[j]) + 1);
                strcpy(args->ignored_files[arr_len], argv[j]);
                arr_len++;
            }
            args->num_ignored_files = arr_len;
        } else if (strcmp(argv[i], "-g") == 0) {
            // printf("found option -g\n");
        }
    }

    if (args->query == NULL && args->path != NULL) {
        args->query = malloc(strlen(args->path) + 1);
        strcpy(args->query, args->path);
        args->path = realloc(args->path, 2 * sizeof(char));
    }

    // Always on, for now
    args->smart_ignore = 1;

    return 1;
}

