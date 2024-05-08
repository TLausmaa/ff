#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include <fts.h>

#define MAX_THREADS   200
#define NO_MATCH      0
#define PARTIAL_MATCH 1
#define EXACT_MATCH   2

const char* query;
int query_len = 0;

pthread_mutex_t threads_mutex;
pthread_mutex_t results_mutex;

_Atomic int threads_created          = 0;
_Atomic int thread_count             = 0;
_Atomic int thread_creation_finished = 0;

struct strarr {
    char** arr;
    int count;
};

struct search_args {
    char* path;
    int depth;
    int is_thread;
};

struct results_t {
    int p_count;
    int e_count;
    int p_cap;
    int e_cap;
    char** partial;
    char** exact;
};

struct exec_args {
    char*  path;
    char*  query;
    char** ignored_files;
    int    num_ignored_files;
};

struct exec_args args;
struct results_t results;

const int NUM_IGNORED_DIRS = 5;
const char* ignored_dirs[] = {
    ".",
    "..",
    ".git",
    ".yarn",
    "node_modules",
};

int is_ignored_filename(const char* filename) {
    for (int i = 0; i < args.num_ignored_files; i++) {
        int patternlen = strlen(args.ignored_files[i]);
        int fnlen  = strlen(filename);
        if (patternlen > fnlen) {
            continue;
        }
        int found = 1;
        for (int j = fnlen - patternlen; j < fnlen; j++) {
            int k = j - (fnlen - patternlen);
            if (filename[j] != args.ignored_files[i][k]) {
                found = 0;
                break;
            }
        }
        if (found) {
            return found;
        }
    }
    return 0;
}

int check_for_match(const char* fn) {
    if (is_ignored_filename(fn)) {
        return NO_MATCH;
    }
    
    char* index = strstr(fn, args.query);

    if (index == NULL) {
        return NO_MATCH;
    }

    if (strlen(fn) == query_len) {
        return EXACT_MATCH;
    } else {
        return PARTIAL_MATCH; 
    }
}

void str_arr_add(char** paths, int* count, char* path, char* filename) {
    char filepath[1024];
    snprintf(filepath, sizeof(filepath), "%s/%s", path, filename);
    paths[*count] = malloc(strlen(filepath) + 1);
    strcpy(paths[*count], filepath);
    paths[*count][strlen(filepath)] = '\0';
    (*count)++;
}

void* searchdir(void* args) {
    char* path = ((struct search_args*)args)->path;
    int depth = ((struct search_args*)args)->depth;
    int is_thread = ((struct search_args*)args)->is_thread;

    DIR* dir;
    struct dirent* entry;

    int retries = 0;
    while ((dir = opendir(path)) == NULL) {
        if (errno == EMFILE) {
            if (retries == 100) {
                return NULL;
            }
            usleep(1000 * 5);
            retries++;
        } else {
            return NULL;
        }
    }

    char** dirs = NULL;
    int dir_count = 0;
    int dir_cap = 10;
    dirs = malloc(sizeof(char*) * dir_cap);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type == DT_DIR) {
            char skip = 0;
            for (int i = 0; i < NUM_IGNORED_DIRS; i++) {
                if (strcmp(entry->d_name, ignored_dirs[i]) == 0) {
                    skip = 1;
                    break;
                }
            }
            if (skip) {
                continue;
            }

            str_arr_add(dirs, &dir_count, path, entry->d_name);
            if (dir_count == dir_cap) {
                dir_cap += 10;
                dirs = realloc(dirs, sizeof(char*) * dir_cap);
            }
        } else {
            int res = check_for_match(entry->d_name);
            if (res != NO_MATCH) {
                pthread_mutex_lock(&results_mutex);
                
                char** list = NULL;
                int* count = NULL;
                int* cap = NULL;

                if (res == PARTIAL_MATCH) {
                    list = results.partial;
                    count = &results.p_count;
                    cap = &results.p_cap;
                } else {
                    list = results.exact;
                    count = &results.e_count;
                    cap = &results.e_cap;
                }

                str_arr_add(list, count, path, entry->d_name);
                if (*count == *cap) {
                    *cap *= 2;
                    list = realloc(list, sizeof(char*) * *cap);
                    if (list == NULL) {
                        printf("realloc failed\n");
                        pthread_mutex_unlock(&results_mutex);
                        closedir(dir);
                        exit(EXIT_FAILURE);
                    }
                    if (res == PARTIAL_MATCH) {
                        results.partial = list;
                    } else {
                        results.exact = list;
                    }
                }

                pthread_mutex_unlock(&results_mutex);
            }
        }
    }
    
    closedir(dir);

    for (int i = 0; i < dir_count; i++) {
        struct search_args* args = malloc(sizeof(struct search_args));
        args->path = dirs[i];
        args->depth = depth + 1;
        args->is_thread = 0;

        if (threads_created < MAX_THREADS) {
            args->is_thread = 1;
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, searchdir, args);
            thread_count++;
            threads_created++;
        } else {
            thread_creation_finished = 1;
            searchdir(args);
        }
    }

    if (is_thread == 1) {
        pthread_mutex_lock(&threads_mutex);
        thread_count--;
        pthread_mutex_unlock(&threads_mutex);
    }

    return NULL;
}

void init_results(void) {
    results.p_count = 0;
    results.e_count = 0;
    results.p_cap = 50;
    results.e_cap = 50;
    results.partial = malloc(sizeof(char*) * results.p_cap);
    results.exact = malloc(sizeof(char*) * results.e_cap);
}

void print_results(void) {
    for (int i = 0; i < results.p_count; i++) {
        printf("%s\n", results.partial[i]);
    }

    if (results.e_count > 0) {
        printf("----------------------------------------\n");
    }

    for (int i = 0; i < results.e_count; i++) {
        printf("%s\n", results.exact[i]);
    }
}

int parse_args(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <query>\n", argv[0]);
        fprintf(stderr, "       %s <path> <query>\n", argv[0]);
        return 0;
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] != '-') {
            if (args.path == NULL) {
                args.path = malloc(strlen(argv[i]) + 1);
                strcpy(args.path, argv[i]);
                continue;
            }
            if (args.query == NULL) {
                args.query = malloc(strlen(argv[i]) + 1);
                strcpy(args.query, argv[i]);
                continue;
            }
        }
        
        if (strcmp(argv[i], "-e") == 0) {
            if (i == argc - 1) {
                printf("Missing parameters for option -e\n");
            }
            char* ignored_files = argv[i+1];
            char* tok = strtok(ignored_files, ",");
            args.ignored_files = malloc(sizeof(char*) * 0);
            int arr_len = 0;
            while (tok != NULL) {
                args.ignored_files = realloc(args.ignored_files, sizeof(char*) * (arr_len + 1));
                args.ignored_files[arr_len] = malloc(strlen(tok) + 1);
                strcpy(args.ignored_files[arr_len], tok);
                arr_len++;
                tok = strtok(NULL, ",");
            }
            args.num_ignored_files = arr_len;
            i++;
        }
    }

    if (args.query == NULL && args.path != NULL) {
        args.query = malloc(strlen(args.path) + 1);
        strcpy(args.query, args.path);
        args.path = realloc(args.path, 2 * sizeof(char));
        strcpy(args.path, ".");
    }

    return 1;
}

int main(int argc, char** argv) {
    if (!parse_args(argc, argv)) {
        printf("parsing args failed\n");
    } 

    query_len = strlen(args.query);

    struct search_args* sargs = malloc(sizeof(struct search_args));
    sargs->path = args.path;
    sargs->depth = 0;
    sargs->is_thread = 0;

    init_results();
    searchdir(sargs);

    while (thread_count > 0 || thread_creation_finished == 0) {
        usleep(1000 * 5);
    }

    print_results();
    return 0;
}
