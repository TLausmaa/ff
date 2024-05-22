#pragma once

#define NO_MATCH      0
#define PARTIAL_MATCH 1
#define EXACT_MATCH   2

struct dir_stats_t {
    int num_ts;
    int num_js;
    int num_c;
};

struct results_t {
    int p_count;
    int e_count;
    int p_cap;
    int e_cap;
    char** partial;
    char** exact;
    struct dir_stats_t* stats;
};

struct exec_args_t {
    char*  path;
    char*  query;
    char** ignored_files;
    int    num_ignored_files;
};

extern const char* query;
extern int query_len;
extern int query_has_uppercase;
extern struct exec_args_t exec_args;

int parse_args(int argc, char** argv, struct exec_args_t* args);
int is_ignored_filename(const char* filename);
int check_for_match(char* filename);
void count_file_type(char* filename);
void print_results(struct results_t* results);
void print_help(int argc, char** argv);
