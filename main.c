#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include <fts.h>

const char* query;
int query_len = 0;

#define MAX_THREADS 50

#define NO_MATCH      0
#define PARTIAL_MATCH 1
#define EXACT_MATCH   2

pthread_t threads[MAX_THREADS] = { NULL };

struct search_args {
    char* path;
    int depth;
};

struct results_t {
    int p_count;
    int e_count;
    int p_cap;
    int e_cap;
    char** partial;
    char** exact;
};

struct results_t results;
pthread_mutex_t results_mutex;

const int NUM_IGNORED_DIRS = 5;
const char* ignored_dirs[] = {
    ".",
    "..",
    ".git",
    "node_modules",
    "yarn"
};

// 0 = no match, 1 = partial match, 2 = exact match
int check_for_match(const char* fn) {
    char* index = strstr(fn, query);
    if (index != NULL) {
        if (strlen(fn) == query_len) {
            return EXACT_MATCH;
        } else {
            return PARTIAL_MATCH; 
        }
    } else {
        return NO_MATCH; 
    }
}

void* searchdir(void* args) {
    char* path = ((struct search_args*)args)->path;
    int depth = ((struct search_args*)args)->depth;

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

            char pathbuf[1024];
            snprintf(pathbuf, sizeof(pathbuf), "%s/%s", path, entry->d_name);
            
            dirs[dir_count] = malloc(strlen(pathbuf) + 1);
            strcpy(dirs[dir_count], pathbuf);
            dirs[dir_count][strlen(pathbuf)] = '\0';
            dir_count++;

            if (dir_count == dir_cap) {
                dir_cap += 10;
                dirs = realloc(dirs, sizeof(char*) * dir_cap);
            }
        } else {
            int res = check_for_match(entry->d_name);
            if (res > NO_MATCH) {
                // printf("%s/%s\n", path, entry->d_name);
                char name[1024];
                snprintf(name, sizeof(name), "%s/%s", path, entry->d_name);

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

                list[*count] = malloc(strlen(name) + 1);
                strcpy(list[*count], name);
                list[*count][strlen(name)] = '\0';
                (*count)++;
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
        if (depth <= 2 && i < MAX_THREADS) {
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, searchdir, args);
            threads[i] = thread_id;
        } else {
            searchdir(args);
        }
    }

    return NULL;
}

int init_results() {
    results.p_count = 0;
    results.e_count = 0;
    results.p_cap = 50;
    results.e_cap = 50;
    results.partial = malloc(sizeof(char*) * results.p_cap);
    results.exact = malloc(sizeof(char*) * results.e_cap);
    return 0;
}

int main(int argc, char** argv) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <path> <query>\n", argv[0]);
        return 1;
    }

    char* path;
    if (argc == 2) {
        query = argv[1];
        path = ".";
    } else {
        path = argv[1];
        query = argv[2];
    }

    query_len = strlen(query);

    struct search_args* args = malloc(sizeof(struct search_args));
    args->path = path;
    args->depth = 0;

    init_results();

    searchdir(args);

    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] != NULL) {
            pthread_join(threads[i], NULL);
        }
    }

    for (int i = 0; i < results.p_count; i++) {
        printf("%s\n", results.partial[i]);
    }

    if (results.e_count > 0) {
        printf("---------------------\n");
    }

    for (int i = 0; i < results.e_count; i++) {
        printf("%s\n", results.exact[i]);
    }

    return 0;
}
