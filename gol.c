#include "utils.h"

/**
 * @brief Region struct.
 *
 * This struct will be used to send the data needed for each process to perform
 * it's work.
 */
typedef struct region {
  // store the coordinates of the starting and end cells
  int min_line;
  int max_line;

  // contains the lines for the process to check
  cell_t** lines;
} region;


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

    MPI_Init(&argc, &argv);

    for (int i = 0; i < steps; i++) {

        MPI_Barrier(MPI_COMM_WORLD);
    }

    MPI_Finalize();



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
 * @brief Gets regions based on input board and sizes.
 *
 * This objects are used to pass the data needed between father and child
 * processes. Ps. this algorithm that I created is stupid. But it works...
 *
 * @param board Regions depend on the board.
 * @param board_size Self-explanatory.
 * @param total_regions Number of regions to return.
 *
 * @return Pointer to array of regions of size defined by the regions parameter.
 */
region* get_regions(cell_t** board, int board_size, int total_regions) {
    region* regions = malloc(sizeof(region) * total_regions);

    // the first part of the algorithm takes the coordinates of each struct
    for (int i = 0; i < total_regions; i++) {
        regions[i].max_line = 0;
        regions[i].min_line = 0;
    }

    int count = 0;
    while (count < board_size) {
        regions[count%n].max_line++;
        count++;
    }

    for (int i = 1; i < n; i++) {
        regions[i].min_line = regions[i-1].max_line;
        regions[i].max_line += regions[i-1].max_line;
    }

    // this loop copies the board part into the struct
    for (int i = 0; i < total_regions; i++) {
        cell_t* region_board[region_size];

        int min_copy = regions[i].min_line;
        if (min_copy > 0)
            min_copy--;

        int max_copy = regions[i].max_line;
        if (max_line < (board_size - 1))
            max_copy++;

        // we need the max_copy line aswell
        for (int j = min_copy; j <= max_copy; j++)
            region_board[i] = board[i];

        reg[i].lines = region_board;
    }

    return regions;
}

/**
 * @brief 'Plays' the game of life.
 *
 * @param board Board to be analysed.
 * @param newboard Temporary board where the just calculated generation will be
 *    stored.
 * @param size Boards' size.
 */
 void play(cell_t** board, cell_t** newboard, int size, int steps) {
    // get rank of process
    int rank;
    MPI_Comm_Rank(MPI_COMM_WORLD, &rank);

    int size;
    MPI_Comm_Size(MPI_COMM_WORLD, &size);
    size--;  // to remove the parent process from the counting

    // parent
    if (rank == 0) {

        // define ranges


        // send signals for child processes
        for (int i = 0; i < size; i++)
            MPI_Send();

        // switch
        tmp = next;
        next = prev;
        prev = tmp;

    // child
    } else {
        // more debug stuff
        #ifdef DEBUG
            printf("%d ----------\n", i + 1);
            print(next, size);
        #endif
    }

    for (int i = 0; i < size; i++)
        for (int j = 0; j < size; j++) {
            a = adjacent_to(board, size, i, j);
            if (a == 2)
                newboard[i][j] = board[i][j];
            if (a == 3)
                newboard[i][j] = 1;
            if (a < 2)
                newboard[i][j] = 0;
            if (a > 3)
                newboard[i][j] = 0;
    }
}

