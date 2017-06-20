#include "utils.h"

inline int adjacent_to(cell_t** board, int size, int i, int j);
unsigned char* int_to_char(int num);
region* get_regions(cell_t** board, int board_size, int total_regions);
unsigned char* serialize_region(region* reg, int* size);
void play(cell_t** board, int size);


int** get_regions_dimensions(int lines, int cols, int regions) {
    // alloc arrays
    int* sizes = malloc(sizeof(int*) * regions);
    for (int i = 0; i < regions; i++)
        sizes[i] = malloc(sizeof(int) * 2);

    int total_lines = lines / total_regions;
    int remainder_lines = lines % total_regions;

    for (int i = 0; i < regions; i++) {
        if (remainder_lines > 0)
            sizes[i][0] = total_lines + 1;
        sizes[i][1] = cols;
    }

    return sizes;
}

cell_t** get_regions(cell_t** board, int lines, int cols, int regions) {
    int total_lines = lines / total_regions;
    int remainder_lines = lines % total_regions;

    for (int i = 0; i < regions; i++) {

    }
    int start_line = index * total_lines;
    if (remainder_lines > 0) {
        total_lines++;
        start_line += index;
    }

    int end_line = start_line + total_lines;
    if (end_line < board_size) {
        total_lines++;
        end_line++;
    }

    int total_cells = total_lines * board_size;
    cell_t* serial_region_board = malloc(sizeof(cell_t*) * total_cells);

    // copies board region
    for (int i = start_line; i < end_line; i++)
        for (int j = 0; j < board_size; j++) {
            int coordinate = ((i - start_line) * board_size) + j;
            serial_region_board[coordinate] = board[i][j];
        }

    // add eveything to struct
    region r;
    r.serial_board = serial_region_board;
    r.lines = total_lines;
    r.cols = board_size;

    return r;
}

cell_t** prepare_board(cell_t** serial, int lines, int cols) {
    // allocate board
    cell_t** board = allocate_board(lines + 2, cols + 2);

    // deserialize board
    int total_cells = lines * cols;
    for (int i = 0; i < total_cells; i++) {
        int line_coord = (i / cols) + 1;
        int col_coord = (i % cols) + 1;
        board[line_coord][col_coord] = serial[i];
    }

    // zeroeing borders
    for (int i = 0; i < lines + 2; i++) {
        board[i][cols + 1] = 0;
        board[i][0] = 0;
    }
    for (int i = 0; i < cols + 2; i++) {
        board[0][i] = 0;
        board[lines + 1][i] = 0;
    }

    return board;
}

cell_t* serialize_board(cell_t** matrix_board, int lines, int cols) {
    int total_cells = lines * cols;

    cell_t* serial_board = malloc(sizeof(cell_t*) * total_cells);

    for (int i = 0; i < total_cells; i++)
        serial_board[i] = matrix_board[i / cols][i % cols];

    return serial_board;
}

int main(int argc, char** argv) {

    FILE* f;
    f = stdin;

    int board_size, steps;
    fscanf(f, "%d %d", &board_size, &steps);

    cell_t** prev = allocate_board(board_size, board_size);
    read_file(f, prev, board_size);

    fclose(f);

    cell_t** next = allocate_board(board_size, board_size);
    cell_t** tmp;


    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int comm_size;
    MPI_Comm_size(MPI_COMM_WORLD, &comm_size);


    if (rank == 0) {

        // send everything
        for (int i = 1; i < comm_size; i++) {
            // send board size and steps number
            region r = get_region_serial(prev, board_size, i-1, comm_size-1);
            int board_properties[3];
            board_properties[0] = r.lines;
            board_properties[1] = r.cols;
            board_properties[2] = steps;
            MPI_Send(board_properties, 3, MPI_INT, i, 0, MPI_COMM_WORLD);

            // send board
            int total_cells = r.lines * r.cols;
            MPI_Send(r.serial_board, total_cells, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
            free(r.serial_board);
        }

        // allocate borders
        cell_t** borders = allocate_board(comm_size - 1, board_size * 2);

        while (steps > 0) {

            // receive borders
            MPI_Status st;
            for (int i = 1; i < comm_size; i++)
                MPI_Recv(borders[i - 1], board_size * 2, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD, &st);

            // send borders
            for (int i = 1; i < comm_size; i++) {
                // border regions
                if (i == 1 || i == (comm_size - 1))
                    MPI_Send(borders[i-1], board_size, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
                else
                    MPI_Send(borders[i-1], board_size*2, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);
            }
            steps--;
        }
        free(borders);

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

    // slave
    } else {
        MPI_Status st;

        // receive board sizes and steps to perform
        int buff[3];
        MPI_Recv(buff, 3, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);
        int lines = buff[0];
        int cols = buff[1];
        steps = buff[2];

        // receive board
        int total_cells = lines * cols;
        cell_t serial_board[total_cells];
        MPI_Recv(serial_board, total_cells, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);

        cell_t** board = prepare_board(serial_board, lines, cols);
        cell_t** temp_board = allocate_board(lines + 2, cols + 2);

        for (int i = 2; i < lines + 2; i++) {
            for (int j = 2; j < cols + 2 ; j++)
                printf ("%c", board[i][j] ? 'x' : ' ');
            printf ("%d\n", rank);
        }

        // real parallel part
        while (steps > 0) {

            // do the game thingy
            for (int i = 2;  i < lines; i++)
                for (int j = 2; j < cols; j++) {
                    int a = adjacent_to((cell_t**) board, 0, i, j);
                    if (a < 2 || a > 3)
                        temp_board[i][j] = 0;
                    if (a == 2)
                        temp_board[i][j] = board[i][j];
                    if (a == 3)
                        temp_board[i][j] = 1;
                }

            // send the borders
            if (rank > 1 && rank < (comm_size - 1)) {
                cell_t borders[cols * 2];
                for (int i = 1; i <= cols; i++) {
                    borders[i - 1] = temp_board[2][i];
                    borders[i - 1 + cols] = temp_board[lines - 1][i];
                }
                MPI_Send(borders, cols * 2, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);

            } else {
                cell_t borders[cols];
                for (int i = 1; i <= cols; i++) {
                    if (rank == 1)
                        borders[i - 1] = temp_board[2][i];
                    else
                        borders[i - 1 + cols] = temp_board[lines - 1][i];
                }
                MPI_Send(borders, cols, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
            }


            // receive borders
            if (rank > 1 && rank < (comm_size - 1)) {
                cell_t borders[cols * 2];
                MPI_Recv(borders, cols * 2, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);

                // copy borders to boards
                for (int i = 0; i < cols; i++) {
                    temp_board[1][i] = borders[i];
                    temp_board[lines - 2][i] = borders[i + cols];
                }

            } else {
                cell_t border[cols];
                MPI_Recv(border, cols, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);

                // copy border into board
                for (int i = 0; i < cols; i++)
                    if (rank == 1)
                        temp_board[1][i] = border[i];
                    else
                        temp_board[lines - 2][i] = border[i];
            }

            // switch boards
            cell_t** t = temp_board;
            temp_board = board;
            board = temp_board;

            steps--;
        }


        for (int i = 2; i < lines + 2; i++) {
            for (int j = 2; j < cols + 2 ; j++)
                printf ("%c", board[i][j] ? 'x' : ' ');
            printf ("\n");
        }


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

    // // debug stuff
    // #ifdef DEBUG
    //     printf("Initial:\n");
    //     print(prev, board_size);
    // #endif



    // #ifdef RESULT
    //     printf("Final:\n");
    //     print (prev, board_size);
    // #endif

    free_board(prev, board_size);
    free_board(next, board_size);
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
