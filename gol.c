#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <mpi.h>


typedef unsigned char cell_t;


cell_t** allocate_board(int lines, int cols) {
    cell_t** board = (cell_t**) malloc(sizeof(cell_t*) * lines);
    for (int i = 0; i < lines; i++)
        board[i] = (cell_t *) malloc(sizeof(cell_t) * cols);
    return board;
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


void free_board(cell_t** board, int size) {
    for (int i = 0; i < size; i++)
        free(board[i]);
    free(board);
}


void read_file(FILE* f, cell_t** board, int size) {
    char* s = (char*) malloc(size + 10);
    fgets(s, size + 10, f);

    for (int j = 0; j < size; j++) {
        fgets(s, size + 10, f);
        for (int i = 0; i < size; i++)
            board[i][j] = s[i] == 'x';
        }
    free(s);
}



void print_board(cell_t** board, int lines, int cols) {
    for (int j = 0; j < lines; j++) {
        for (int i = 0; i < cols; i++)
            printf("%c", board[j][i] ? 'x' : ' ');
        printf("|\n");
    }
}


/**
 * @brief Return dimensions of the board based on this program's approach.
 */
int** get_sending_region_dimensions(int lines, int cols, int regions) {
    // alloc arrays
    int** sizes = malloc(sizeof(int*) * regions);
    for (int i = 0; i < regions; i++)
        sizes[i] = malloc(sizeof(int) * 2);

    // do calculations
    int lines_per_region = lines / regions;
    int remainder_lines = lines % regions;

    for (int i = 0; i < regions; i++) {
        int region_lines = lines_per_region;
        if (remainder_lines > 0) {
            region_lines++;
            remainder_lines--;
        }

        sizes[i][1] = cols;

        // middle regions
        if (i > 0 && i < regions - 1)
            sizes[i][0] = region_lines + 2;
        // first or last regions
        else
            sizes[i][0] = region_lines + 1;
    }

    return sizes;
}


/**
 * @brief Simple free of matrices.
 *
 * @param matrix Pointer to matrix.
 * @param lines Total lines of matrix.
 */
void free_matrix(void** matrix, int lines) {
    for (int i = 0; i < lines; i++)
        free(matrix[i]);
    free(matrix);
}


/**
 * @brief Get regions of the board to send.
 *
 * It uses the logic of sending the region it is going to be calculated plus
 * the borders needed to for the calculations. The regions are made in lines,
 * only. This was chosen for the simplicity. The algorithm to separate borders
 * for any kind of separations would be too complex and time consuming.
 *
 * @param board Where to get the regions from.
 * @param lines Size of the original board.
 * @param cols Size of the original board.
 * @param regions How many regions we want to divide the original board.
 *
 * @return A pointer to a 3D array with the regions of the boards. The size of
 * each of this regions is returned by the function
 * 'get_sending_regions_dimensions'.
 */
cell_t*** get_sending_regions(cell_t** board, int lines, int cols, int regions) {
    cell_t*** boards = malloc(sizeof(cell_t**) * regions);
    int** dims = get_sending_region_dimensions(lines, cols, regions);

    int region_start = 0;

    // top region (+1) for the top border of this region
    boards[0] = malloc(sizeof(cell_t*) * dims[0][0]);
    for (int i = 0; i < dims[0][0]; i++) {
        boards[0][i] = malloc(sizeof(cell_t) * dims[0][1]);
        memcpy(boards[0][i], board[i], dims[0][1]);
    }
    region_start += dims[0][0]-1;

    for (int i = 1; i < regions - 1; i++) {
        boards[i] = malloc(sizeof(cell_t*) * dims[i][0]);

        for (int j = 0; j < dims[i][0]; j++) {
            boards[i][j] = malloc(sizeof(cell_t) * dims[i][1]);
            memcpy(boards[i][j], board[region_start+j-1], dims[i][1]);
        }
        region_start += dims[i][0]-1;
    }

    boards[regions-1] = malloc(sizeof(cell_t*) * dims[regions-1][0]);
    for (int i = 0; i < dims[regions-1][0]; i++) {
        boards[regions-1][i] = malloc(sizeof(cell_t) * dims[regions-1][1]);
        // very ugly coordinates...
        memcpy(boards[regions-1][i], board[lines-dims[regions-1][0]+i], dims[regions-1][1]);
    }

    // free the dims 2d array
    free_matrix((void**) dims, regions);

    return boards;
}


void zero_borders(cell_t** board, int lines, int cols) {
    // zeroeing borders
    for (int i = 0; i < lines; i++) {
        board[i][cols-1] = 0;
        board[i][0] = 0;
    }
    for (int i = 0; i < cols; i++) {
        board[0][i] = 0;
        board[lines-1][i] = 0;
    }
}


cell_t** prepare_board(cell_t* serial, int lines, int cols) {
    // allocate board
    cell_t** board = allocate_board(lines+2, cols+2);

    // deserialize board
    int total_cells = lines * cols;
    for (int i = 0; i < total_cells; i++) {
        int line_coord = (i / cols) + 1;
        int col_coord = (i % cols) + 1;
        board[line_coord][col_coord] = serial[i];
    }

    zero_borders(board, lines+2, cols+2);

    return board;
}


/**
 * @brief Simple function that serializes a matrix of cells into one big array.
 *
 * @param matrix_board What to serialize.
 * @param lines Matrix size.
 * @param cols Matrix size.
 *
 * @returns Pointer to array of the serialized matrix of size (lines * cols).
 */
cell_t* serialize_board(cell_t** matrix_board, int lines, int cols) {
    int total_cells = lines * cols;

    cell_t* serial_board = malloc(sizeof(cell_t) * total_cells);
    for (int i = 0; i < total_cells; i++)
        serial_board[i] = matrix_board[i / cols][i % cols];

    return serial_board;
}


/**
 * @brief Simple function to allocate the borders.
 *
 * These borders are the borders that the child processes send back after each
 * iteration of the Game of Life.
 * Note that we do not need the number of lines because this implementation
 * only uses lines to separate regions. Then, we only need the number of regions
 * to return the correct size of borders.
 *
 * @param regions Number of regions that are being calculated.
 * @param cols Number of coluns in each region.
 *
 * @return Pointer to 2D array of borders. Note that the middle items of this
 * array are different in size of the ones in the borders. That is so because
 * the edge regions (top and bottom) only have one border each.
 */
cell_t** allocate_borders(int regions, int cols) {
    cell_t** borders = malloc(sizeof(cell_t*) * regions);

    // first and last regions only have one border each
    borders[0] = malloc(sizeof(cell_t) * cols);
    borders[regions-1] = malloc(sizeof(cell_t) * cols);

    for (int i = 1; i < regions-1; i++)
        borders[i] = malloc(sizeof(cell_t) * 2 * cols);
}


/**
 * It will sedn to everybody, except master.
 *
 * @param buff Pointer to data to send.
 * @param buff_size Size of the data to send.
 * @param comm_size Size of the comm, including master.
 *
 * @returns Pointer to array of request to be used later in an MPI_Wait.
 */
MPI_Request* send_to_all_non_blocking(int* buff, int buff_size, int comm_size) {
    MPI_Request* reqs = malloc(sizeof(MPI_Request) * comm_size-1);
    for (int i = 0; i < comm_size-1; i++) {
        int dest = i + 1;
        MPI_Isend(buff, buff_size, MPI_INT, dest, 0, MPI_COMM_WORLD, &reqs[i]);
    }

    return reqs;
}


/**
 *
 * @brief Wait for all request to complete.
 *
 * @param reqs Array of request to wait for completion.
 * @param size Reqs' size.
 *
 * @return Pointer to arrau of status of all the sends.
 */
MPI_Status* wait_for_all(MPI_Request* reqs, int size) {
    MPI_Status* st = malloc(sizeof(MPI_Status) * size);

    for (int i = 0; i < size; i++)
        MPI_Wait(&reqs[i], &st[i]);

    return st;
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
 *
 * @returns Saves the new board state into the temp board.
 */
void play(cell_t** curr, cell_t** temp, int lines, int cols) {
    // do the game thingy
    for (int i = 2;  i < lines; i++)
        for (int j = 2; j < cols; j++) {
            printf("(%d, %d)\n", i, j);
            int a = adjacent_to(curr, i, j);
            if (a < 2 || a > 3)
                temp[i][j] = 0;
            else if (a == 3)
                temp[i][j] = 1;
            else if (a == 2)
                temp[i][j] = curr[i][j];
        }
}


int main(int argc, char** argv) {

    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int comm_size;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);


    if (rank == 0) {
        FILE* f;
        f = stdin;

        int board_size, steps;
        fscanf(f, "%d %d", &board_size, &steps);

        // send steps to each process (non blocking)
        MPI_Request* steps_reqs = send_to_all_non_blocking(&steps, 1, comm_size);

        printf("TESTE\n");

        cell_t** prev = allocate_board(board_size, board_size);
        read_file(f, prev, board_size);

        fclose(f);

        cell_t** next = allocate_board(board_size, board_size);
        cell_t** tmp;

        // debug stuff
        #ifdef DEBUG
            printf("Initial:\n");
            print_board(prev, board_size);
        #endif


        // get dimensions
        int** dims = get_sending_region_dimensions(board_size, board_size, comm_size-1);
        // send board dimensions to everybody
        MPI_Request dim_reqs[comm_size-1];
        for (int i = 0; i < comm_size-1; i++) {
            int dest = i + 1;
            MPI_Isend(dims[i], 2, MPI_INT, dest, 0, MPI_COMM_WORLD, &dim_reqs[i]);
        }


        // separate board
        cell_t*** regions = get_sending_regions(prev, board_size, board_size, comm_size-1);
        // send board to children
        for (int i = 0; i < comm_size-1; i++) {
            cell_t* serial_board = serialize_board(regions[i], dims[i][0], dims[i][1]);
            free_matrix((void**) regions[i], dims[i][0]);


            // wait for the process we are sending to have
            // the board dimensions already
            MPI_Status st;
            MPI_Wait(&dim_reqs[i], &st);


            // send board itself
            int dest = i + 1;
            int cells = dims[i][0] * dims[i][1];
            MPI_Send(serial_board, cells, MPI_UNSIGNED_CHAR, dest, 0, MPI_COMM_WORLD);
            free(serial_board);
        }




        // // allocate borders
        // cell_t** borders = allocate_board(comm_size - 1, board_size * 2);

        // while (steps > 0) {

        //     // receive borders
        //     MPI_Status st;
        //     for (int i = 1; i < comm_size; i++)
        //         MPI_Recv(borders[i - 1], board_size * 2, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, &st);

        //     // send borders
        //     for (int i = 1; i < comm_size; i++) {
        //         // border regions
        //         if (i == 1 || i == (comm_size - 1))
        //             MPI_Send(borders[i-1], board_size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
        //         else
        //             MPI_Send(borders[i-1], board_size*2, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
        //     }
        //     steps--;
        // }
        // free(borders);

        // // receive final boards
        // for (int i = 1; i < comm_size; i++) {
        //     MPI_Status st;
        //     int lines_cols[2];
        //     MPI_Recv(lines_cols, 2, MPI_INT, i, 0, MPI_COMM_WORLD, &st);

        //     int total_cells = lines_cols[0] * lines_cols[1];
        //     cell_t serial_board[total_cells];
        //     MPI_Recv(serial_board, total_cells, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, &st);

        //     int src = i - 1;
        //     for (int j = 0; j < lines_cols[0]; j++) {
        //         for (int k = 0; k < lines_cols[1]; k++)
        //             printf ("%c", serial_board[(j * lines_cols[1]) + k] ? 'x' : ' ');
        //         printf ("\n");
        //     }
        // }
        //
        //
        //
        //
        free_board(prev, board_size);
        free_board(next, board_size);

    // slave
    } else {
        int steps;
        MPI_Status step_st;
        MPI_Recv(&steps, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &step_st);


        // debug
        printf("Child %d received steps (%d)\n", rank, steps);


        // receive board sizes and steps to perform
        int dims[2];
        MPI_Status dims_st;
        MPI_Recv(dims, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &dims_st);

        // for better readability
        int lines = dims[0];
        int cols = dims[1];


        // debug
        printf("Child %d received dims(%d, %d)\n", rank, dims[0], dims[1]);


        // allocate temp board
        cell_t** temp = allocate_board(lines + 2, cols + 2);
        zero_borders(temp, lines+2, cols+2);

        // receive board
        MPI_Status board_st;
        int total_cells = lines * cols;
        cell_t* serial_board = malloc(sizeof(cell_t) * total_cells);
        MPI_Recv(serial_board, total_cells, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &board_st);


        // debug
        printf("Child %d board received\n", rank);
        // for (int i = 0; i < lines; i++) {
        //     for (int j = 0; j < cols; j++)
        //         printf("%c", serial_board[i*cols + j] ? 'x' : '_');
        //     printf("\n");
        // }

        cell_t** board = prepare_board(serial_board, lines, cols);
        free(serial_board);


        // debug
        printf("Child %d board ready\n", rank);
        print_board(temp, lines+2, cols+2);


        // // real parallel part
        while (steps > 0) {
            play(board, temp, lines, cols);

            // print_board(temp, lines+2, cols+2);
            break;
        }



        //     // send the borders
        //     if (rank > 1 && rank < (comm_size - 1)) {
        //         cell_t borders[cols * 2];
        //         for (int i = 1; i <= cols; i++) {
        //             borders[i - 1] = temp_board[2][i];
        //             borders[i - 1 + cols] = temp_board[lines - 1][i];
        //         }
        //         MPI_Send(borders, cols * 2, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);

        //     } else {
        //         cell_t borders[cols];
        //         for (int i = 1; i <= cols; i++) {
        //             if (rank == 1)
        //                 borders[i - 1] = temp_board[2][i];
        //             else
        //                 borders[i - 1 + cols] = temp_board[lines - 1][i];
        //         }
        //         MPI_Send(borders, cols, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
        //     }


        //     // receive borders
        //     if (rank > 1 && rank < (comm_size - 1)) {
        //         cell_t borders[cols * 2];
        //         MPI_Recv(borders, cols * 2, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);

        //         // copy borders to boards
        //         for (int i = 0; i < cols; i++) {
        //             temp_board[1][i] = borders[i];
        //             temp_board[lines - 2][i] = borders[i + cols];
        //         }

        //     } else {
        //         cell_t border[cols];
        //         MPI_Recv(border, cols, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);

        //         // copy border into board
        //         for (int i = 0; i < cols; i++)
        //             if (rank == 1)
        //                 temp_board[1][i] = border[i];
        //             else
        //                 temp_board[lines - 2][i] = border[i];
        //     }

        //     // switch boards
        //     cell_t** t = temp_board;
        //     temp_board = board;
        //     board = temp_board;

        //     steps--;
        // }


        // for (int i = 2; i < lines + 2; i++) {
        //     for (int j = 2; j < cols + 2 ; j++)
        //         printf ("%c", board[i][j] ? 'x' : ' ');
        //     printf ("\n");
        // }


        // total_cells = (lines - 2) * (cols - 2);
        // cell_t final_serial_board[total_cells];
        // // serialize final board to send
        // for (int i = 2; i < lines + 2; i++)
        //     for (int j = 2; j < cols + 2; j++) {
        //         int serial_coord = (i - 2) * cols;
        //         serial_coord += j - 2;
        //         final_serial_board[serial_coord] = board[i][j];
        //     }

        // int lines_cols[2] = {lines - 2, cols - 2};
        // MPI_Send(lines_cols, 2, MPI_INT, 0, 0, MPI_COMM_WORLD);
        // MPI_Send(final_serial_board, total_cells, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);

        // arrumar isso aqui [TODO]
        // free_board(board, lines + 2);
        // free_board(temp_board, lines + 2);
    }

    MPI_Finalize();




    // // #ifdef RESULT
    // //     printf("Final:\n");
    // //     print_board (prev, board_size);
    // // #endif


}
