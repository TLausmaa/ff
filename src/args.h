#pragma once

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
