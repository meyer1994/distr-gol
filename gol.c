#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

typedef unsigned char cell_t;


// prototypes
cell_t** allocate_board(int lines, int cols);
void free_board(cell_t** board, int size);
void read_file(FILE* f, cell_t** board, int size);
void print_board(cell_t** board, int lines, int cols, int rank);
void play(cell_t** board, cell_t** newboard, int lines, int cols);
inline int adjacent_to(cell_t** board, int i, int j);

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

// needs refactoring hahaha (2am folks...)
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

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int c_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);

    int c_size;
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    cell_t** matrix_board;

    // ========================================================================

    // read file
    int steps_size_send_buff[2];
    if (c_rank == 0) {
        FILE* board_file = stdin;
        fscanf(board_file, "%d %d", &steps_size_send_buff[1], &steps_size_send_buff[0]);

        steps_size_send_buff[1] += 2;  // +2 because of the zeroed borders
        int board_size = steps_size_send_buff[1];

        matrix_board = allocate_board(board_size, board_size);
        read_file(board_file, matrix_board, board_size-2);

        fclose(board_file);
    }

    MPI_Bcast(steps_size_send_buff, 2, MPI_INT, 0, MPI_COMM_WORLD);
    int steps = steps_size_send_buff[0];
    int board_size = steps_size_send_buff[1];
    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        if (c_rank == 0)
            printf("\n");
        printf("P%d received b_steps (%d) and board_size (%d) from root\n", c_rank, steps, board_size);
    #endif

    // ========================================================================

    // get ranges
    int* send_ranges;
    if (c_rank == 0)
        send_ranges = get_ranges(board_size, c_size);

    int range[2];
    MPI_Scatter(send_ranges, 2, MPI_INT, range, 2, MPI_INT, 0, MPI_COMM_WORLD);

    if (c_rank == 0)
        free(send_ranges);
    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
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
        MPI_Barrier(MPI_COMM_WORLD);
        printf("P%d received expandend_ranges (%d, %d) from root\n", c_rank, expandend_ranges[0], expandend_ranges[1]);
        printf("P%d lines and cols (%d, %d)\n", c_rank, lines, cols);
    #endif

    // ========================================================================

    int regions_offsets[c_size];
    int regions_counts[c_size];
    cell_t* serial_region_board;
    if (c_rank == 0) {
        serial_region_board = serialize_board(matrix_board, board_size, board_size);
        for (int i = 0; i < c_size; i++) {
            regions_offsets[i] = send_expanded_ranges[i*2] * board_size;

            int r_lines = send_expanded_ranges[i*2+1] - send_expanded_ranges[i*2];
            regions_counts[i] = r_lines * board_size;
        }
    }

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
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

    // ========================================================================

    cell_t* serial_board = malloc(sizeof(cell_t) * cells);
    MPI_Scatterv(serial_region_board, regions_counts, regions_offsets, MPI_UNSIGNED_CHAR, serial_board, cells, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        printf("\n");
        printf("P%d received serial_board from root\n", c_rank);
        for (int i = 0; i < lines; i++) {
            for (int j = 0; j < cols; j++)
                printf ("%s", serial_board[i*cols+j] ? "x " : "- ");
            printf("| %d\n", c_rank);
        }
    #endif

    // ========================================================================

    // the problem is in this line
    cell_t** temp_board = allocate_board(lines, cols);
    cell_t** game_board = deserialize_board(serial_board, lines, cols);
    print_board(game_board, lines, cols, c_rank);
    free(serial_board);

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        printf("\n");
        printf("P%d's board is ready\n", c_rank);
        // print_board(game_board, lines, cols, c_rank);
    #endif

    // ========================================================================

    // while (b_steps > 0) {
    //     // play(p_prep_board, p_temp_board, p_lines, board_size);
    //     // printf("P%d finished a step (%d)\n", c_rank, b_steps);
    //     // print_board(p_temp_board, p_lines+extra_lines, board_size+2, c_rank);
    //     break;
    // }

    // for (int i = 0; i < steps; i++) {
    // play(prev, next, size);
    // #ifdef DEBUG
    // printf("%d ----------\n", i + 1);
    // print(next, size);
    // #endif
    // tmp = next;
    // next = prev;
    // prev = tmp;
    // }

    // #ifdef RESULT
    //     printf("Final:\n");
    //     print (prev,size);
    // #endif

    // free_board(prev,size);
    // free_board(next,size);

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
    // printf("(%d, %d)\n", i, j);
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
    for (int i = 1; i <= lines; i++) {
        for (int j = 1; j <= cols; j++) {
            // printf("%d, %d\n", i, j);
            int a = adjacent_to(board, i, j);
            printf("%d ", a);
            // if (a == 2)
            //     newboard[i][j] = board[i][j];
            // if (a == 3)
            //     newboard[i][j] = 1;
            // if (a < 2)
            //     newboard[i][j] = 0;
            // if (a > 3)
            //     newboard[i][j] = 0;
        }
        printf("\n");
    }
}

void print_board(cell_t** board, int lines, int cols, int rank) {
    for (int j = 0; j < lines; j++) {
        for (int i = 0; i < cols; i++)
            printf ("%s", board[j][i] ? " x" : " -");
        printf ("| [P%d]\n", rank);
    }
}

void read_file(FILE* f, cell_t** board, int size) {
    char* s = (char*) malloc(size + 10);
    fgets(s, size + 10, f);
    for (int j = 0; j < size; j++) {
        fgets (s, size + 10, f);
        for (int i = 0; i < size; i++)
            board[i+1][j+1] = s[i] == 'x';
    }
}
