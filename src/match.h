#pragma once

#define NO_MATCH      0
#define PARTIAL_MATCH 1
#define EXACT_MATCH   2

int is_ignored_filename(const char* filename);
int check_for_match(char* filename);
