#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>

#include "main.h"

#define MAX_THREADS   200

const char* query;
int query_len           = 0;
int query_has_uppercase = 0;
struct exec_args_t exec_args;

pthread_mutex_t threads_mutex;
pthread_mutex_t results_mutex;

_Atomic int threads_created = 0;
_Atomic int thread_count    = 0;

struct search_args_t {
    char* path;
    int depth;
    int is_thread;
};

struct results_t results;

const int NUM_IGNORED_DIRS = 5;
const char* ignored_dirs[] = {
    ".",
    "..",
    ".git",
    ".yarn",
    "node_modules",
};

_Atomic int qcount = 0;
pthread_mutex_t q_mutex;

void add_q(void) {
    pthread_mutex_lock(&q_mutex);
    qcount++;
    pthread_mutex_unlock(&q_mutex);
}

void pop_q(void) {
    pthread_mutex_lock(&q_mutex);
    qcount--;
    pthread_mutex_unlock(&q_mutex);
}

void str_arr_add(char** paths, int* count, char* path, char* filename) {
    char filepath[1024];
    if (strcmp(path, ".") == 0) {
        snprintf(filepath, sizeof(filepath), "%s", filename);
    } else {
        snprintf(filepath, sizeof(filepath), "%s/%s", path, filename);
    }
    paths[*count] = malloc(strlen(filepath) + 1);
    strcpy(paths[*count], filepath);
    paths[*count][strlen(filepath)] = '\0';
    (*count)++;
}

void* searchdir(void* args) {
    char* path = ((struct search_args_t*)args)->path;
    int depth = ((struct search_args_t*)args)->depth;
    int is_thread = ((struct search_args_t*)args)->is_thread;

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
            // count_file_type(entry->d_name);
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
        struct search_args_t* new_args = malloc(sizeof(struct search_args_t));
        new_args->path = dirs[i];
        new_args->depth = depth + 1;
        new_args->is_thread = 0;

        if (threads_created < MAX_THREADS) {
            add_q();
            thread_count++;
            threads_created++;
            new_args->is_thread = 1;
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, searchdir, new_args);
        } else {
            searchdir(new_args);
        }
    }

    if (is_thread == 1) {
        pthread_mutex_lock(&threads_mutex);
        thread_count--;
        pthread_mutex_unlock(&threads_mutex);
        pop_q();
    }

    return NULL;
}

void init_results(void) {
    results.p_count = 0;
    results.e_count = 0;
    results.p_cap   = 50;
    results.e_cap   = 50;
    results.partial = malloc(sizeof(char*) * results.p_cap);
    results.exact   = malloc(sizeof(char*) * results.e_cap);
    results.stats   = malloc(sizeof(struct dir_stats_t));
    results.stats->num_ts = 0;
    results.stats->num_js = 0;
    results.stats->num_c  = 0;
}

int main(int argc, char** argv) {
    if (!parse_args(argc, argv, &exec_args)) {
        print_help(argc, argv);
        return 0;
    } 
    
    query_len = strlen(exec_args.query);
    for (int i = 0; i < query_len; i++) {
        if (isupper(exec_args.query[i])) {
            query_has_uppercase = 1;
            break;
        }
    }

    struct search_args_t* sargs = malloc(sizeof(struct search_args_t));
    sargs->path = exec_args.path;
    sargs->depth = 0;
    sargs->is_thread = 0;

    init_results();
    searchdir(sargs);

    while (qcount > 0) {
        usleep(1000 * 5);
    }

    print_results(&results);
    return 0;
}
