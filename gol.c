#include "utils.h"

// default size to serialize ints
#define INT_CHAR_SIZE 10

typedef struct region {
    // coordinate of where to start calculations
    int begin_x;
    int begin_y;

    // number of cells to calculate
    int total_cells;

    // the board where to do it
    cell_t** board;
    // board properties
    int lines;
    int columns;
} region;


inline int adjacent_to(cell_t** board, int size, int i, int j);
unsigned char* int_to_char(int num);
region* get_regions(cell_t** board, int board_size, int total_regions);
unsigned char* serialize_region(region* reg, int* size);
void play(cell_t** board, int size, int steps);


int main(int argc, char** argv) {

    FILE* f;
    f = stdin;

    int size, steps;
    fscanf(f, "%d %d", &size, &steps);

    cell_t** prev = allocate_board(size);
    read_file(f, prev, size);

    fclose(f);

    cell_t** next = allocate_board(size);
    cell_t** tmp;

    // debug stuff
    #ifdef DEBUG
        printf("Initial:\n");
        print(prev, size);
    #endif



    #ifdef RESULT
        printf("Final:\n");
        print (prev,size);
    #endif

    free_board(prev,size);
    free_board(next,size);
}


/**
 * @brief Checks for alive cells around the coordinate.
 *
 * TODO: some performance improvment
 *
 * @param board Matrix to check upon.
 * @param size Matrix' size.
 * @param i Line coordinate.
 * @param j Column coordinate.
 *
 * @return Total alive cells around coordinate.
 */
inline int adjacent_to(cell_t** board, int size, int i, int j) {
    int count = 0;
    if (i-1 > 0) {
        count += board[i-1][j];
        if (j - 1 > 0)
            count += board[i-1][j-1];
        if (j+1 < size)
            count += board[i-1][j+1];
    }

    if (j-1 > 0) {
        count += board[i][j-1];
        if (i+1 < size)
            count += board[i+1][j-1];
    }

    if (i+1 < size) {
        count += board[i+1][j];
        if (j+1 < size)
            count += board[i+1][j+1];
    }

    if (j+1 < size)
        count += board[i][j+1];

    return count;
}

/**
 * @brief Simple function to convert a int into a string (char*).
 *
 * The size of the string/char* will be defined by the value in INT_CHAR_SIZE.
 *
 * @param num Int to be converted.
 *
 * @return Char array with the number.
 */
unsigned char* int_to_char(int num) {
    unsigned char* c = malloc(sizeof(unsigned char) * INT_CHAR_SIZE);

    sprintf(c, "%d", num);
    return c;
}


/**
 * @brief Gets regions based on input board and sizes.
 *
 * It will return an struct with the properties needed to pass to each process.
 *
 * @param board Board to get regions from.
 * @param board_size Self-explanatory.
 * @param total_regions Number of regions to return.
 *
 * @return Pointer to array of regions of size defined by the total_regions
 * parameter.
 */
region* get_regions(cell_t** board, int board_size, int total_regions) {

    region* regions = malloc(sizeof(region) * total_regions);

    // for some reason, sometimes the board size would misteriously change to 0
    // so I assing the columns here first
    for (int i = 0;  i < total_regions; i++)
        regions[i].columns = board_size;

    // sets regions' total cells
    int total_cells = board_size * board_size;
    int cells_per_region = total_cells / total_regions;
    int remain_cells = total_cells % total_regions;
    for (int i = 0; i < total_regions; i++) {
        regions[i].total_cells = cells_per_region;
        if (remain_cells > 0) {
            regions[i].total_cells++;
            remain_cells--;
        }
    }


    // sets beginning coordinates of each region
    int cell_coord = 0;
    for (int i = 0; i < total_regions; i++) {
        regions[i].begin_x = cell_coord % board_size;
        regions[i].begin_y = cell_coord / board_size;
        cell_coord += regions[i].total_cells;
    }



    // sets board, and board properties, for each region
    for (int i = 0; i < total_regions; i++) {
        // gets up border of board
        int begin_line = regions[i].begin_y;
        if (begin_line > 0)
            begin_line--;

        // gets down border of board
        int end_line = regions[i].begin_y + (regions[i].total_cells / regions[i].columns);
        if (end_line < board_size)
            end_line++;


        // sets number of lines
        regions[i].lines = end_line - begin_line;


        // copies the board into the struct
        cell_t** region_board = malloc(sizeof(cell_t*) * regions[i].lines * board_size);
        for (int j = begin_line; j < end_line; j++)
            region_board[j] = board[j];

        regions[i].board = region_board;


        // sets line properties to be based on the local board
        regions[i].begin_y -= begin_line;
    }

    return regions;
}

/**
 * NOT FINISHED!!
 * @brief Serialize the region.
 *
 * @param reg Pointer to region to be serialized.
 * @param size Pointer to int where to store the total_size of the serilized
 *      array.
 *
 * @return Pointer to serialized array.
 */
unsigned char* serialize_region(region* reg, int* size) {
    int total_size = reg->lines * reg->columns;  // board size
    int ints_offset = 5 * INT_CHAR_SIZE;  // ints
    total_size += ints_offset;

    unsigned char* serialized = malloc(sizeof(unsigned char) * total_size);

    // convert ints to chars
    unsigned char* min_line_c = int_to_char(reg->min_line);
    unsigned char* max_line_c = int_to_char(reg->max_line);
    unsigned char* lines_c = int_to_char(reg->lines);
    unsigned char* columns_c = int_to_char(reg->columns);

    // copies the converted ints to the char array
    for (int i = 0; i < INT_CHAR_SIZE; i++) {
        serialized[1 * INT_CHAR_SIZE + i] = min_line_c[i];
        serialized[0 * INT_CHAR_SIZE + i] = max_line_c[i];
        serialized[2 * INT_CHAR_SIZE + i] = lines_c[i];
        serialized[3 * INT_CHAR_SIZE + i] = columns_c[i];
    }

    // Dobby is freeeee!!
    free(min_line_c);
    free(max_line_c);
    free(lines_c);
    free(columns_c);

    // i am so sorry for this...
    for (int i = 0; i < reg->lines; i++)
        for (int j = 0; j < reg->columns; j++)
            serialized[ints_offset + (i * reg->columns) + j];

    // set value and return
    *size = total_size;
    return serialized;
}

