#pragma once

#define NO_MATCH      0
#define PARTIAL_MATCH 1
#define EXACT_MATCH   2

typedef struct dir_stats_t {
    int num_ts;
    int num_js;
    int num_c;
} dir_stats_t;

typedef struct results_t {
    int p_count;
    int e_count;
    int p_cap;
    int e_cap;
    char** partial;
    char** exact;
    dir_stats_t* stats;
} results_t;

typedef struct exec_args_t {
    char*  path;
    char*  query;
    char** ignored_files;
    int    num_ignored_files;
    int    smart_ignore;
} exec_args_t;

extern const char* query;
extern int query_len;
extern int query_has_uppercase;
extern exec_args_t exec_args;

int parse_args(int argc, char** argv, exec_args_t*);
int is_ignored_filename(const char* filename);
int check_for_match(char* filename);
void count_file_type(char* filename, results_t*);
void print_results(results_t*);
void print_help(int argc, char** argv);
