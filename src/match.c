#include <string.h>
#include <ctype.h>

#include "args.h"
#include "match.h"

int is_ignored_filename(const char* filename) {
    for (int i = 0; i < exec_args.num_ignored_files; i++) {
        int patternlen = strlen(exec_args.ignored_files[i]);
        int fnlen  = strlen(filename);
        if (patternlen > fnlen) {
            continue;
        }
        int found = 1;
        for (int j = fnlen - patternlen; j < fnlen; j++) {
            int k = j - (fnlen - patternlen);
            if (filename[j] != exec_args.ignored_files[i][k]) {
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

int check_for_match(char* filename) {
    char* fn = filename;
    int fnlen = strlen(fn);
    
    if (is_ignored_filename(fn)) {
        return NO_MATCH;
    }
    
    char fnlower[fnlen + 1];
    if (!query_has_uppercase) {
        // if query is written in all lowercase, assume case-insensitive search
        // and convert filename to lowercase as well
        for (int i = 0; i < fnlen; i++) {
            fnlower[i] = tolower(fn[i]);
        }
        fnlower[fnlen] = '\0';
        fn = fnlower;
    }
    
    char* index = strstr(fn, exec_args.query);

    if (index == NULL) {
        return NO_MATCH;
    }

    if (fnlen == query_len) {
        return EXACT_MATCH;
    } else {
        return PARTIAL_MATCH; 
    }
}

