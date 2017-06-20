#include "utils.h"

typedef struct region {
    cell_t* serial_board;
    int lines;
    int cols;
} region;


inline int adjacent_to(cell_t** board, int size, int i, int j);
unsigned char* int_to_char(int num);
region* get_regions(cell_t** board, int board_size, int total_regions);
unsigned char* serialize_region(region* reg, int* size);
void play(cell_t** board, int size);


region get_region_serial(cell_t** board, int board_size, int index, int total_regions) {
    int total_lines = board_size / total_regions;
    int remainder_lines = board_size % total_regions;

    int start_line = index * total_lines;
    if (remainder_lines - index > 0) {
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

cell_t** prepare_board(region r) {
    // allocate board
    cell_t** board = allocate_board(r.lines + 2, r.cols + 2);

    // deserialize board
    int total_cells = r.lines * r.cols;
    for (int i = 0; i < total_cells; i++) {
        int line_coord = (i / r.cols) + 1;
        int col_coord = (i % r.cols) + 1;
        board[line_coord][col_coord] = r.serial_board[i];
    }

    // zeroeing borders
    for (int i = 0; i < r.lines + 2; i++) {
        board[i][r.cols + 1] = 0;
        board[i][0] = 0;
    }
    for (int i = 0; i < r.cols + 2; i++) {
        board[0][i] = 0;
        board[r.lines + 1][i] = 0;
    }

    return board;
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
            region r = get_region_serial(prev, board_size, i-1, comm_size-1);
            int board_properties[2];
            board_properties[0] = r.lines;
            board_properties[1] = r.cols;
            MPI_Send(board_properties, 2, MPI_INT, i, 0, MPI_COMM_WORLD);

            int total_cells = r.lines * r.cols;
            MPI_Send(r.serial_board, total_cells, MPI_UNSIGNED_CHAR, i, 0, MPI_COMM_WORLD);

            // used malloc on it
            free(r.serial_board);
        }

        // allocate borders
        cell_t** borders = allocate_board(comm_size - 1, board_size * 2);

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
            free(borders[i-1]);
       }
       free(borders);
       steps--;



    // slave
    } else {
        MPI_Status st;

        region reg_s;
        int board_properties[2];
        MPI_Recv(board_properties, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);
        reg_s.lines = board_properties[0];
        reg_s.cols = board_properties[1];


        int total_cells = reg_s.lines * reg_s.cols;
        cell_t region_board_serial[total_cells];
        MPI_Recv(region_board_serial, total_cells, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);
        reg_s.serial_board = region_board_serial;


        cell_t** board = prepare_board(reg_s);
        cell_t** temp_board = allocate_board(reg_s.lines + 2, reg_s.cols + 2);

        // do the game thingy
        for (int i = 2;  i < reg_s.lines; i++)
            for (int j = 2; j < reg_s.cols; j++) {
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
            cell_t borders[reg_s.cols * 2];
            for (int i = 1; i <= reg_s.cols; i++) {
                borders[i - 1] = temp_board[2][i];
                borders[i - 1 + reg_s.cols] = temp_board[reg_s.lines - 1][i];
            }
            MPI_Send(borders, reg_s.cols * 2, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);

        } else {
            cell_t borders[reg_s.cols];
            for (int i = 1; i <= reg_s.cols; i++) {
                if (rank == 1)
                    borders[i - 1] = temp_board[2][i];
                else
                    borders[i - 1 + reg_s.cols] = temp_board[reg_s.lines - 1][i];
            }
            MPI_Send(borders, reg_s.cols, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD);
        }

        // receive borders
        if (rank > 1 && rank < (comm_size - 1)) {
            cell_t borders[reg_s.cols * 2];
            MPI_Recv(borders, reg_s.cols * 2, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);

            // copy borders to boards
            for (int i = 0; i < reg_s.cols; i++) {
                temp_board[1][i] = borders[i];
                temp_board[reg_s.lines - 2][i] = borders[i + reg_s.cols];
            }

        } else {
            cell_t border[reg_s.cols];
            MPI_Recv(border, reg_s.cols, MPI_UNSIGNED_CHAR, 0, 0, MPI_COMM_WORLD, &st);

            // copy border into board
            for (int i = 0; i < reg_s.cols; i++)
                if (rank == 1)
                    temp_board[1][i] = border[i];
                else
                    temp_board[reg_s.lines - 2][i] = border[i];
        }

        // switch boards
        cell_t** t = temp_board;
        temp_board = board;
        board = temp_board;

        steps--;
    }


    MPI_Finalize();

    // debug stuff
    #ifdef DEBUG
        printf("Initial:\n");
        print(prev, board_size);
    #endif



    #ifdef RESULT
        printf("Final:\n");
        print (prev, board_size);
    #endif

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
