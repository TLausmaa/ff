#include <strings.h>
#include <stdlib.h>
#include <stdio.h>

#include "main.h"

void parse_following_args(int argc, char** argv, int start, int* num_parsed, char*** args_res) {
    if (start > argc - 1) {
        printf("Missing parameters for option %s\n", argv[start - 1]);
        exit(EXIT_FAILURE);
    }
    int arr_len = 0;
    for (int j = start; j < argc; j++) {
        if (argv[j][0] == '-') {
            break;
        }
        *args_res = realloc(*args_res, sizeof(char*) * (arr_len + 1));
        (*args_res)[arr_len] = malloc(strlen(argv[j]) + 1);
        strcpy((*args_res)[arr_len], argv[j]);
        arr_len++;
    }
    *num_parsed = arr_len;
}

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
       
        if (strcmp(argv[i], "-o") == 0) {
            parse_following_args(argc, argv, i + 1, &args->num_exclusive_files, &args->exclusive_files);
        } else if (strcmp(argv[i], "-i") == 0) {
            parse_following_args(argc, argv, i + 1, &args->num_ignored_files, &args->ignored_files);
        } else if (strcmp(argv[i], "-g") == 0) {
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

