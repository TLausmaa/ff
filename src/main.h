#pragma once

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
