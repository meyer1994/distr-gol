#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mpi.h>

typedef unsigned char cell_t;


// prototypes
void free_board(cell_t** board, int lines);
cell_t** allocate_board(int lines, int cols);
void read_file(FILE* f, cell_t* board, int size);
inline int adjacent_to(cell_t** board, int i, int j);
int* get_range(int lines, int rank, int size);
cell_t* serialize_board(cell_t** board, int lines, int cols);
void print_board(cell_t** board, int lines, int cols, int rank);
void play(cell_t** board, cell_t** newboard, int lines, int cols);
cell_t** deserialize_board(cell_t* serial_board, int lines, int cols);
void print_serial(cell_t* serial_board, int lines, int cols, int rank);
void exchange_borders(cell_t** board, int lines, int cols, int c_size, int c_rank);



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

    // get real ranges
    int* range = get_range(board_size, c_rank, c_size);
    int lines = range[1] - range[0];
    int cols = board_size;
    int cells = cols * lines;
    #ifdef DEBUG
        printf("P%d received range (%d, %d) from root\n", c_rank, range[0], range[1]);
        printf("P%d total lines and cols (%d, %d)\n", c_rank, lines, cols);
    #endif

    // ========================================================================

    int regions_offsets[c_size];
    int regions_counts[c_size];
    if (c_rank == 0) {
        for (int i = 0; i < c_size; i++) {
            int* p_range = get_range(board_size, i, c_size);
            regions_offsets[i] = p_range[0] * board_size;

            int r_lines = p_range[1] - p_range[0];
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

    // ========================================================================

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
    free(serial_region_board);

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

        exchange_borders(temp_board, lines, cols, c_size, c_rank);

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

    serial_region_board = serialize_board(game_board, lines, cols);
    // commented for performance
    // free_board(temp_board, lines);
    // free_board(game_board, lines);

    MPI_Gatherv(serial_region_board, cells, MPI_UNSIGNED_CHAR, file_board, regions_counts, regions_offsets, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    // print everything and finish
    if (c_rank == 0) {
        printf("Final:\n");
        for (int i = 0; i < board_size-2; i++) {
            for (int j = 0; j < board_size-2; j++)
                printf ("%c", file_board[(i+1)*(board_size)+j+1] ? 'x' : ' ');
            printf ("\n");
        }
        // commented for performance
        // free(file_board);
    }

    MPI_Finalize();
}

cell_t** allocate_board(int lines, int cols) {
    cell_t** board = malloc(sizeof(cell_t*) * lines);
    for (int i = 0; i < lines; i++)
        board[i] = calloc(sizeof(cell_t), cols);
    return board;
}

void free_board(cell_t** board, int lines) {
    for (int i = 0; i < lines; i++)
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

int* get_range(int lines, int rank, int size) {
    int* range = malloc(sizeof(int) * 2);
    int reg_lines = lines / size;
    int rem_lines = lines % size;

    int start_line = 0;
    for (int i = 0; i < rank; i++) {
        start_line += reg_lines;
        if (rem_lines > 0) {
            start_line++;
            rem_lines--;
        }
    }

    int end_line = start_line + reg_lines;
    if (rem_lines > 0)
        end_line++;

    if (start_line - 1 > 0)
        start_line--;
    if (end_line + 1 < lines)
        end_line++;

    range[0] = start_line;
    range[1] = end_line;

    return range;
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

void exchange_borders(cell_t** board, int lines, int cols, int c_size, int c_rank) {
    if (c_rank > 0)
        MPI_Send(board[1], cols, MPI_UNSIGNED_CHAR, c_rank-1, 0, MPI_COMM_WORLD);

    MPI_Status top_recv_st;
    if (c_rank < c_size-1)
        MPI_Recv(board[lines-1], cols, MPI_UNSIGNED_CHAR, c_rank+1, 0, MPI_COMM_WORLD, &top_recv_st);

    if (c_rank < c_size-1)
        MPI_Send(board[lines-2], cols, MPI_UNSIGNED_CHAR, c_rank+1, 1, MPI_COMM_WORLD);

    MPI_Status bot_recv_st;
    if (c_rank > 0)
        MPI_Recv(board[0], cols, MPI_UNSIGNED_CHAR, c_rank-1, 1, MPI_COMM_WORLD, &bot_recv_st);
}

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