#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

typedef unsigned char cell_t;


// prototypes
int* get_ranges(int lines, int regions);
void free_board(cell_t** board, int size);
cell_t** allocate_board(int lines, int cols);
int* get_real_ranges(int lines, int regions);
void read_file(FILE* f, cell_t* board, int size);
inline int adjacent_to(cell_t** board, int i, int j);
cell_t* serialize_board(cell_t** board, int lines, int cols);
void print_board(cell_t** board, int lines, int cols, int rank);
void play(cell_t** board, cell_t** newboard, int lines, int cols);
cell_t** deserialize_board(cell_t* serial_board, int lines, int cols);

void print_serial(cell_t* serial_board, int lines, int cols, int rank) {
    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < cols; j++)
            printf ("%c", serial_board[i*cols+j] ? 'x' : '-');
        if (rank > -1)
            printf ("| [%d]\n", rank);
        else
            printf ("|\n");
    }
}

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int c_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);

    int c_size;
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    cell_t* file_board;

    // ========================================================================

    // read file
    int steps_size_send_buff[2];
    if (c_rank == 0) {
        FILE* board_file = stdin;
        fscanf(board_file, "%d %d", &steps_size_send_buff[1], &steps_size_send_buff[0]);

        steps_size_send_buff[1] += 2;  // +2 because of the zeroed borders
        int board_size = steps_size_send_buff[1];

        file_board = calloc(1, board_size*board_size);
        read_file(board_file, file_board, board_size-2);
        fclose(board_file);
    }

    MPI_Bcast(steps_size_send_buff, 2, MPI_INT, 0, MPI_COMM_WORLD);
    int steps = steps_size_send_buff[0];
    int board_size = steps_size_send_buff[1];
    #ifdef DEBUG
        printf("P%d received b_steps (%d) and board_size (%d) from root\n", c_rank, steps, board_size);
    #endif

    // ========================================================================

    // get ranges
    int* send_ranges;
    if (c_rank == 0)
        send_ranges = get_ranges(board_size, c_size);

    int range[2];
    MPI_Scatter(send_ranges, 2, MPI_INT, range, 2, MPI_INT, 0, MPI_COMM_WORLD);

    #ifdef DEBUG
        printf("P%d received ranges (%d, %d) from root\n", c_rank, range[0], range[1]);
    #endif

    // ========================================================================

    // get real ranges
    int* send_expanded_ranges;
    if (c_rank == 0)
        send_expanded_ranges = get_real_ranges(board_size, c_size);

    int expandend_ranges[2];
    MPI_Scatter(send_expanded_ranges, 2, MPI_INT, expandend_ranges, 2, MPI_INT, 0, MPI_COMM_WORLD);
    int lines = expandend_ranges[1] - expandend_ranges[0];
    int cols = board_size;
    int cells = cols * lines;
    #ifdef DEBUG
        printf("P%d received expandend_ranges (%d, %d) from root\n", c_rank, expandend_ranges[0], expandend_ranges[1]);
        printf("P%d total lines and cols (%d, %d)\n", c_rank, lines, cols);
    #endif

    // ========================================================================

    int regions_offsets[c_size];
    int regions_counts[c_size];
    if (c_rank == 0) {
        for (int i = 0; i < c_size; i++) {
            regions_offsets[i] = send_expanded_ranges[i*2] * board_size;

            int r_lines = send_expanded_ranges[i*2+1] - send_expanded_ranges[i*2];
            regions_counts[i] = r_lines * board_size;
        }
    }

    #ifdef DEBUG
        if (c_rank == 0) {
            printf("\noffsets [");
            for (int i = 0; i < c_size; i++)
                printf(" %2d", regions_offsets[i]);
            printf(" ]\n counts [");
            for (int i = 0; i < c_size; i++)
                printf(" %2d", regions_counts[i]);
            printf(" ]\n");
        }
    #endif

    // // ========================================================================

    cell_t* serial_region_board = malloc(sizeof(cell_t) * cells);
    MPI_Scatterv(file_board, regions_counts, regions_offsets, MPI_UNSIGNED_CHAR, serial_region_board, cells, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    #ifdef DEBUG
        printf("\n");
        printf("P%d received serial_region_board from root\n", c_rank);
        print_serial(serial_region_board, lines, cols, c_rank);
    #endif

    // ========================================================================

    cell_t** temp_board = allocate_board(lines, cols);
    cell_t** game_board = deserialize_board(serial_region_board, lines, cols);
    // free(serial_region_board);  // commented for performance

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        printf("\n");
        printf("P%d's board is ready\n", c_rank);
        // print_board(game_board, lines, cols, c_rank);
    #endif

    // ========================================================================

    while (steps > 0) {
        play(game_board, temp_board, lines, cols);

        #ifdef DEBUG
            printf("P%d finished a step (%d)\n", c_rank, steps);
            // print_board(temp_board, lines, cols, c_rank);
        #endif

        if (c_rank > 0)
            MPI_Send(temp_board[1], cols, MPI_UNSIGNED_CHAR, c_rank-1, 0, MPI_COMM_WORLD);

        MPI_Status top_recv_st;
        if (c_rank < c_size-1)
            MPI_Recv(temp_board[lines-1], cols, MPI_UNSIGNED_CHAR, c_rank+1, 0, MPI_COMM_WORLD, &top_recv_st);

        if (c_rank < c_size-1)
            MPI_Send(temp_board[lines-2], cols, MPI_UNSIGNED_CHAR, c_rank+1, 1, MPI_COMM_WORLD);

        MPI_Status bot_recv_st;
        if (c_rank > 0)
            MPI_Recv(temp_board[0], cols, MPI_UNSIGNED_CHAR, c_rank-1, 1, MPI_COMM_WORLD, &bot_recv_st);

        #ifdef DEBUG
            // if (c_rank > 0) {
            //     printf("P%d received top border\n", c_rank);
            //     for (int i = 0; i < cols; i++)
            //         printf ("%s", temp_board[0][i] ? " x" : " -");
            //     printf(" [%d]\n", c_rank);
            // }

            // if (c_rank < c_size-1) {
            //     printf("P%d received bot border\n", c_rank);
            //     for (int i = 0; i < cols; i++)
            //         printf ("%s", temp_board[lines-1][i] ? " x" : " -");
            //     printf(" [%d]\n", c_rank);
            // }
        #endif

        // switch
        cell_t** t = game_board;
        game_board = temp_board;
        temp_board = t;

        steps--;
    }

    // ========================================================================

    // commented for performance
    // free(serial_region_board);
    // free(temp_board);
    serial_region_board = serialize_board(game_board, lines, cols);

    MPI_Gatherv(serial_region_board, cells, MPI_UNSIGNED_CHAR, file_board, regions_counts, regions_offsets, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // print everything and finish
    if (c_rank == 0) {
        printf("Final:\n");
        // free(matrix_board);
        for (int i = 0; i < board_size-2; i++) {
            for (int j = 0; j < board_size-2; j++)
                printf ("%c", file_board[(i+1)*(board_size)+j+1] ? 'x' : ' ');
            printf ("\n");
        }
    }

    MPI_Finalize();
}

cell_t** allocate_board(int lines, int cols) {
    cell_t** board = malloc(sizeof(cell_t*) * lines);
    for (int i = 0; i < lines; i++)
        board[i] = calloc(sizeof(cell_t), cols);
    return board;
}

void free_board(cell_t** board, int size) {
    for (int i = 0; i < size; i++)
        free(board[i]);
    free(board);
}

inline int adjacent_to(cell_t** board, int i, int j) {
    int count = 0;

    count += board[i-1][j];
    count += board[i-1][j-1];
    count += board[i-1][j+1];
    count += board[i][j-1];
    count += board[i][j+1];
    count += board[i+1][j-1];
    count += board[i+1][j];
    count += board[i+1][j+1];

    return count;
}

void play(cell_t** board, cell_t** newboard, int lines, int cols) {
    for (int i = 1; i < lines-1; i++) {
        for (int j = 1; j < cols-1; j++) {
            int a = adjacent_to(board, i, j);
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
}

void print_board(cell_t** board, int lines, int cols, int rank) {
    for (int i = 0; i < lines; i++) {
        for (int j = 0; j < cols; j++)
            printf ("%c", board[i][j] ? 'x' : ' ');
        printf ("\n");
    }
}

void read_file(FILE* f, cell_t* board, int size) {
    char s[size+10];
    fgets(s, size + 10, f);
    for (int j = 1; j <= size; j++) {
        fgets (s, size+10, f);
        for (int i = 1; i <= size; i++)
            board[j*(size+2)+i] = s[i-1] == 'x';
    }
}

int* get_ranges(int lines, int regions) {
    int* r = malloc(sizeof(int) * regions * 2);

    int reg_lines = lines / regions;
    int rem_lines = lines % regions;
    int counter = 0;

    for (int i = 0; i < regions; i++) {
        int begin_line = counter;
        int end_line = counter + reg_lines;

        if (rem_lines > 0) {
            rem_lines--;
            end_line++;
        }
        r[2*i] = begin_line;
        r[2*i+1] = end_line;

        counter = end_line;
    }

    return r;
}

int* get_real_ranges(int lines, int regions) {
    int* r_ranges = get_ranges(lines, regions);

    for (int i = 0; i < regions; i++) {
        int start_line = r_ranges[i*2];
        int end_line = r_ranges[i*2+1];

        if (start_line - 1 > 0)
            start_line--;
        if (end_line + 1 < lines)
            end_line++;

        r_ranges[i*2] = start_line;
        r_ranges[i*2+1] = end_line;
    }

    return r_ranges;
}

cell_t* serialize_board(cell_t** board, int lines, int cols) {
    int cells = lines * cols;

    cell_t* serial = malloc(sizeof(cell_t) * cells);

    for (int i = 0; i < lines; i++)
        for (int j = 0; j < cols; j++)
            serial[i*cols+j] = board[i][j];

    return serial;
}

cell_t** deserialize_board(cell_t* serial_board, int lines, int cols) {
    int cells = lines * cols;
    cell_t** prep_board = malloc(sizeof(cell_t*) * cells);
    for (int i = 0; i < lines; i++) {
        prep_board[i] = malloc(sizeof(cell_t) * cols);
        for (int j = 0; j < cols; j++) {
            memcpy(prep_board[i], &serial_board[i*cols], cols);
        }
    }

    return prep_board;
}