#include <stdio.h>
#include <dirent.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <errno.h>

#include <fts.h>

const char* query;

#define MAX_THREADS 50
pthread_t threads[MAX_THREADS] = { NULL };

struct search_args {
    char* path;
    int depth;
};

void* walkdir(void* args) {
    char* path = ((struct search_args*)args)->path;
    int depth = ((struct search_args*)args)->depth;

    DIR* dir;
    struct dirent* entry;

    int retries = 0;
    while ((dir = opendir(path)) == NULL) {
        if (errno == EMFILE) {
            if (retries == 100) {
                printf("Too many retries, exiting...\n");
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
            if (strcmp(entry->d_name, ".") == 0 || 
                strcmp(entry->d_name, "..") == 0 || 
                strcmp(entry->d_name, ".git") == 0 || 
                strcmp(entry->d_name, "node_modules") == 0 || 
                strcmp(entry->d_name, "yarn") == 0 
                ) {
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
            if (strcmp(entry->d_name, query) == 0) {
                printf("Found: %s/%s\n", path, entry->d_name);
            }
        }
    }
    
    closedir(dir);

    for (int i = 0; i < dir_count; i++) {
        struct search_args* args = malloc(sizeof(struct search_args));
        args->path = dirs[i];
        args->depth = depth + 1;
        if (depth <= 1 && i < MAX_THREADS) {
            pthread_t thread_id;
            pthread_create(&thread_id, NULL, walkdir, args);
            threads[i] = thread_id;
        } else {
            walkdir(args);
        }
    }

    return NULL;
}

int fts_find(char* const dir) {
    FTS *ftsp;
    FTSENT *p, *chp;
    int fts_options = FTS_COMFOLLOW | FTS_LOGICAL | FTS_NOCHDIR;

    char** args = NULL;
    args = malloc(sizeof(char*) * 1);
    args[0] = malloc(strlen(dir) + 1);
    strcpy(args[0], dir);

    if ((ftsp = fts_open(args, fts_options, NULL)) == NULL) {
          printf("fts_open failed\n");
          return -1;
    }

    chp = fts_children(ftsp, 0);
    if (chp == NULL) {
          return 0;               /* no files to traverse */
    }
    while ((p = fts_read(ftsp)) != NULL) {
          switch (p->fts_info) {
          case FTS_D:
                  printf("d %s\n", p->fts_path);
                  break;
          case FTS_F:
                  printf("f %s\n", p->fts_path);
                  break;
          default:
                  break;
          }
    }
    fts_close(ftsp);
    return 0;
}

#define CUSTOM 1

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

#ifdef CUSTOM
    printf("Running custom search\n");
    struct search_args* args = malloc(sizeof(struct search_args));
    args->path = path;
    args->depth = 0;

    walkdir(args);

    for (int i = 0; i < MAX_THREADS; i++) {
        if (threads[i] != NULL) {
            pthread_join(threads[i], NULL);
        }
    }
#else
    printf("Running fts search\n");
    fts_find(path);
#endif

    return 0;
}
