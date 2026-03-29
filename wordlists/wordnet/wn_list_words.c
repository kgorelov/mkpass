#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "wn.h"

/* 
   Sanitization criteria:
   - No digits
   - No special characters (dashes, underscores, dots, etc.)
   - No non-ASCII characters
   - Length between 4 and 10 characters (inclusive)
*/
int is_valid_word(const char *word) {
    int len = 0;
    while (word[len] != '\0') {
        unsigned char c = (unsigned char)word[len];
        /* Only allow basic ASCII letters (A-Z, a-z) */
        if (!((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))) {
            return 0;
        }
        len++;
    }
    /* Filter out words too short (<=3) or too long (>10) */
    if (len < 4 || len > 10) {
        return 0;
    }
    return 1;
}

void print_pos(int pos) {
    char line[LINEBUF];
    if (pos < 1 || pos > NUMPARTS || indexfps[pos] == NULL) {
        return;
    }

    /* Rewind to the beginning of the index file */
    rewind(indexfps[pos]);

    while (fgets(line, sizeof(line), indexfps[pos])) {
        /* Skip header lines starting with spaces */
        if (line[0] == ' ' || line[0] == '\n') {
            continue;
        }

        /* The word (lemma) is the first token on the line */
        char *word = strtok(line, " ");
        if (word && is_valid_word(word)) {
            printf("%s\n", word);
        }
    }
}

int main(int argc, char *argv[]) {
    int pos;

    if (wninit() != 0) {
        fprintf(stderr, "Failed to initialize WordNet library.\n");
        fprintf(stderr, "Make sure WNSEARCHDIR is set to the 'dict' directory.\n");
        return 1;
    }

    if (argc > 1) {
        /* Check if a specific POS was requested */
        int requested_pos = StrToPos(argv[1]);
        if (requested_pos == -1) {
            fprintf(stderr, "Usage: %s [noun|verb|adj|adv]\n", argv[0]);
            return 1;
        }
        print_pos(requested_pos);
    } else {
        /* Default: iterate through all parts of speech */
        for (pos = 1; pos <= NUMPARTS; pos++) {
            print_pos(pos);
        }
    }

    return 0;
}
