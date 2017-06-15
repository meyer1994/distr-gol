#include <stdio.h>
#include <stdlib.h>

// copied neede structs
typedef unsigned char cell_t;
typedef struct region {
  // store the coordinates of the starting and end cells
  int min_line;
  int max_line;

  // contains the lines for the process to check
  int x;
  int y;
} region;

// testing the region creator algorithm
int main(int argc, char const *argv[]) {
    // test values
    int total_regions = 4;
    int board_size = 5;

    region* regions = malloc(sizeof(region) * total_regions);

    // the first part of the algorithm takes the coordinates of each struct
    for (int i = 0; i < total_regions; i++) {
        regions[i].max_line = 0;
        regions[i].min_line = 0;
    }

    int count = 0;
    while (count < board_size) {
        regions[count%total_regions].max_line++;
        count++;
    }

    for (int i = 1; i < total_regions; i++) {
        regions[i].min_line = regions[i-1].max_line;
        regions[i].max_line += regions[i-1].max_line;
    }

    // this loop copies the board part into the struct
    for (int i = 0; i < total_regions; i++) {
        int min_copy = regions[i].min_line;
        if (min_copy > 0)
            min_copy--;

        int max_copy = regions[i].max_line;
        if (max_copy < board_size)
            max_copy++;

        regions[i].x = min_copy;
        regions[i].y = max_copy;
    }

    printf("min_l\tmax_l\tmin_c\tmax_c\n");
    for (int i = 0; i < total_regions; i++) {
        printf("%d\t%d\t%d\t%d\n", regions[i].min_line, regions[i].max_line,
            regions[i].x, regions[i].y);
    }

    free(regions);

    return 0;
}