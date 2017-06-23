#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>

#include "gol_utils.h"


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
inline int adjacent_to(cell_t** board, int i, int j) {
    int count = 0;
    // top
    count += board[i-1][j-1];
    count += board[i-1][j];
    count += board[i-1][j+1];

    // left
    count += board[i][j-1];
    // right
    count += board[i][j+1];

    // bottom
    count += board[i+1][j-1];
    count += board[i+1][j];
    count += board[i+1][j+1];

    return count;
}


/**
 * @brief Play the Game of Life.
 *
 * This function assumes that our boards have 2 more lines and columns. This
 * extra lines and columns are used as a 0 border so that we can bypass the
 * many board limit checks to do the calculations.
 *
 * @param curr Current board.
 * @param temp Tempo board. Where the new state of the game will be saved.
 * @param lines Num of lines of the original, without 0 borders, board.
 * @param cols Same as lines, but for the columns.
 * @param start_line It is used to limit the start of the calculations, because,
 *      sometimes, depending on the region (first or last), the regions will
 *      need to use some other lines.
 * @param end_line Same as the start_line, but for the end of the region.
 *
 * @returns Saves the new board state into the temp board.
 */
void play(cell_t** curr, cell_t** temp, int lines, int cols, int start_line, int end_line) {
    // do the game thingy
    for (int i = start_line;  i < end_line; i++)
        for (int j = 1; j <= cols; j++) {
            int a = adjacent_to(curr, i, j);
            if (a < 2 || a > 3)
                temp[i][j] = 0;
            else if (a == 3)
                temp[i][j] = 1;
            else if (a == 2)
                temp[i][j] = curr[i][j];
        }
}


inline int get_total_borders(int rank, int comm_size) {
    int total_borders = 2;
    if (rank == 1 || rank == comm_size-1)
        return 1;
    return total_borders;
}


int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int comm_size;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);


    if (rank == 0) {
        int steps;
        int board_size;
        FILE* f = stdin;
        fscanf(f, "%d %d", &board_size, &steps);

        // broadcast steps
        MPI_Request steps_req;
        MPI_Ibcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD, &steps_req);

        cell_t** current = allocate_board(board_size, board_size);
        cell_t** next;

        read_file(f, current, board_size);
        fclose(f);


        // debug stuff
        #ifdef DEBUG
            printf("Initial:\n";
            print_board(current, board_size);
        #endif


        // get dimensions
        int** dims = get_sending_region_dimensions(board_size, board_size, comm_size-1);
        // send board dimensions to everybody
        MPI_Request dim_reqs[comm_size-1];
        for (int i = 0; i < comm_size-1; i++) {
            int dest = i + 1;
            printf("(%d, %d)\n", dims[i][0], dims[i][1]);
            MPI_Isend(dims[i], 2, MPI_INT, dest, 0, MPI_COMM_WORLD, &dim_reqs[i]);
        }


        // separate board
        cell_t** regions = get_sending_regions(current, board_size, board_size, comm_size-1);


        // // send board to children
        // for (int i = 0; i < comm_size-1; i++) {
        //     // wait for the process we are sending to have
        //     // the board dimensions already
        //     MPI_Status st;
        //     MPI_Wait(&dim_reqs[i], &st);

        //     // send board itself
        //     int dest = i + 1;
        //     int cells = dims[i][0] * dims[i][1];
        //     MPI_Send(serial_board, cells, MPI_UNSIGNED_CHAR, dest, 0, MPI_COMM_WORLD);

        //     // free things
        //     free_matrix((void**) regions[i], dims[i][0]);
        //     free(serial_board);
        // }


        // cell_t** final_region = malloc(sizeof(cell_t*) * comm_size-1);
        // for (int i = 0; i < comm_size-1; i++)
        //     final_region[i] = calloc(dims[i][1] + 2, sizeof(cell_t));


        // for (int i = 0; i < comm_size-1; i++) {
        //     int cells = dims[i][0] * dims[i][1];
        //     cell_t final_region[cells];

        //     int dest = i + 1;

        //     MPI_Status recv_st;
        //     MPI_Recv(final_region, cells, MPI_UNSIGNED_CHAR, dest, 0, MPI_COMM_WORLD, &recv_st);


            // first region
            // int start_line = 2;
            // if (rank == 1)
            //     start_line = 1;
            // // last region
            // int end_line = dims[i][0];
            // if (rank == comm_size-1)
            //     end_line = dims[i][0] - 1;

            // for (int j = start_line; j < end_line; j++) {
            //     for (int k = 1; k <= dims[i][1]; k++)
            //         printf("%c", final_region[j * dims[i][1] + k] ? 'x' : '-');
            //     printf("\n");
            // }
        }

        // free_matrix((void**) prev, board_size);
        // free_matrix((void**) next, board_size);

    // slave
    // } else {

    //     // receive steps
    //     int steps;
    //     MPI_Bcast(&steps, 1, MPI_INT, 0, MPI_COMM_WORLD);


    //     // receive board sizes and steps to perform
    //     int dims[2];
    //     MPI_Status dims_st;
    //     MPI_Recv(dims, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &dims_st);

    //     // for better readability
    //     int lines = dims[0];
    //     int cols = dims[1];


    //     // allocate temp board with all zeroes
    //     cell_t** temp = malloc(sizeof(cell_t*) * (lines + 2));
    //     for (int i = 0; i < lines + 2; i++)
    //         temp[i] = calloc(cols+2, sizeof(cell_t));

    //     // receive board
    //     MPI_Status board_st;
    //     int total_cells = lines * cols;
    //     cell_t* serial_board = malloc(sizeof(cell_t) * total_cells);
    //     MPI_Recv(serial_board, total_cells, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &board_st);


    //     cell_t** board = prepare_board(serial_board, lines, cols);
    //     free(serial_board);

    //     // first region
    //     int start_line = 2;
    //     if (rank == 1)
    //         start_line = 1;
    //     // last region
    //     int end_line = lines;
    //     if (rank == comm_size-1)
    //         end_line = lines - 1;

    //     int total_borders = get_total_borders(rank, comm_size);


    //     // real parallel part
    //     while (steps > 0) {

    //         play(board, temp, lines, cols, start_line, end_line);


    //         // send borders
    //         MPI_Request borders_req[2];
    //         MPI_Status borders_st[2];

    //         // send borders
    //         if (rank > 1)
    //             MPI_Isend(temp[2], cols+2, MPI_UNSIGNED_CHAR, rank-1, 0, MPI_COMM_WORLD, &borders_req[0]);
    //         if (rank < comm_size-1)
    //             MPI_Isend(temp[lines-1], cols+2, MPI_UNSIGNED_CHAR, rank+1, 0, MPI_COMM_WORLD, &borders_req[1]);

    //         // receive borders
    //         if (rank > 1)
    //             MPI_Recv(temp[1], cols+2, MPI_UNSIGNED_CHAR, rank-1, 0, MPI_COMM_WORLD, &borders_st[0]);
    //         if (rank < comm_size-1)
    //             MPI_Recv(temp[lines], cols+2, MPI_UNSIGNED_CHAR, rank+1, 0, MPI_COMM_WORLD, &borders_st[1]);

    //         // wait for other processes to receive info
    //         if (rank > 1)
    //             MPI_Wait(&borders_req[0], &borders_st[0]);
    //         if (rank < comm_size-1)
    //             MPI_Wait(&borders_req[1], &borders_st[1]);


    //         // debug
    //         // printf("Child %d board after with new borders\n", rank);
    //         // print_board(temp, lines+2, cols+2);

    //         // switch time
    //         cell_t** t = board;
    //         board = temp;
    //         temp = t;

    //         // break;
    //         steps--;
    //     }


    //     // deserialize board
    //     cell_t final_board[total_cells];
    //     for (int i = 0; i < lines; i++)
    //         for (int j = 0; j < cols; j++)
    //             final_board[i*cols + j] = board[i+1][j+1];

    //     // send everything
    //     MPI_Request final_req;
    //     MPI_Isend(final_board, total_cells, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &final_req);
    //     free_matrix((void**) board, lines+2);
    //     free_matrix((void**) temp, lines+2);

    //     // wait for send and end
    //     MPI_Status final_st;
    //     MPI_Wait(&final_req, &final_st);
    // }



    MPI_Finalize();




    // // #ifdef RESULT
    // //     printf("Final:\n");
    // //     print_board (prev, board_size);
    // // #endif


}
