#include <stdio.h>
#include <stdlib.h>

int* get_real_ranges(int lines, int rank, int size) {
    int* range = malloc(sizeof(int) * 2);
    int reg_lines = lines / size;
    int rem_lines = lines % size;

    int start_line = 0;
    for (int i = 0; i < rank; i++) {
        start_line += reg_lines;
        if (rem_lines > 0) {
            start_line++;
            rem_lines--;
        }
    }

    int end_line = start_line + reg_lines;
    if (rem_lines > 0)
        end_line++;

    if (start_line - 1 > 0)
        start_line--;
    if (end_line + 1 < lines)
        end_line++;

    range[0] = start_line;
    range[1] = end_line;

    return range;
}

int main(int argc, char const *argv[]) {
    int b = 5000;
    int c = 4;
    for (int i = 0; i < c; i++) {
        int* r = get_real_ranges(b, i, c);
        printf("(%d, %d)\n", r[0], r[1]);
        free(r);
    }


    return 0;
}