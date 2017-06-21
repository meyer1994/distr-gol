#include <stdio.h>
#include <stdlib.h>

/**
 * Teste created to test the method of getting regions dimensions for the
 * control of sub-matrix creations
 */

int** get_regions_dimensions(int lines, int cols, int regions) {
    // alloc arrays
    int** sizes = malloc(sizeof(int*) * regions);
    for (int i = 0; i < regions; i++)
        sizes[i] = malloc(sizeof(int) * 2);

    int lines_per_region = lines / regions;
    int remainder_lines = lines % regions;

    printf("lines_per_region %d\n", lines_per_region);
    printf("remainder_lines %d\n", remainder_lines);

    for (int i = 0; i < regions; i++) {
        int region_lines = lines_per_region;
        if (remainder_lines > 0) {
            region_lines++;
            remainder_lines--;
        }
        sizes[i][0] = region_lines;
        sizes[i][1] = cols;
    }

    return sizes;
}

int main(int argc, char const *argv[])
{
    int** dims = get_regions_dimensions(14, 14, 10);

    for (int i = 0; i < 10; i++)
        printf("Lines: %d, Cols: %d\n", dims[i][0], dims[i][1]);
    return 0;
}