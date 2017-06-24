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
cell_t** prepare_board(cell_t* s_board, int lines, int cols, int rank, int size) {

    // middle sections
    if (rank > 0 && rank < size-1) {
        cell_t** prep_board = malloc(sizeof(cell_t*) * lines);
        for (int i = 0; i < lines; i++) {
            prep_board[i] = malloc(sizeof(cell_t) * (cols + 2));
            prep_board[i][0] = 0;
            prep_board[i][cols+1] = 0;
            memcpy(&prep_board[i][1], &s_board[i*cols], cols);
        }
        return prep_board;

    // only one process
    } else if (size == 1) {
        cell_t** prep_board = malloc(sizeof(cell_t*) * (lines + 2));
        for (int i = 1; i <= lines; i++) {
            prep_board[i] = malloc(sizeof(cell_t) * (cols + 2));
            prep_board[i][0] = 0;
            prep_board[i][cols+1] = 0;
            memcpy(&prep_board[i][1], &s_board[(i-1)*cols], cols);
        }
        prep_board[0] = calloc(sizeof(cell_t), cols + 2);
        prep_board[lines+1] = calloc(sizeof(cell_t), cols + 2);
        return prep_board;

    // top section
    } else if (rank == 0) {
        cell_t** prep_board = malloc(sizeof(cell_t*) * (lines + 1));
        for (int i = 1; i <= lines; i++) {
            prep_board[i] = malloc(sizeof(cell_t) * cols + 2);
            prep_board[i][0] = 0;
            prep_board[i][cols+1] = 0;
            memcpy(&prep_board[i][1], &s_board[(i-1)*cols], cols);
        }
        prep_board[0] = calloc(sizeof(cell_t), cols + 2);
        return prep_board;

    // bottom section
    } else {
        cell_t** prep_board = malloc(sizeof(cell_t*) * (lines + 1));
        for (int i = 0; i <= lines; i++) {
            prep_board[i] = malloc(sizeof(cell_t) * cols + 2);
            prep_board[i][0] = 0;
            prep_board[i][cols+1] = 0;
            memcpy(&prep_board[i][1], &s_board[i*cols], cols);
        }
        prep_board[lines] = calloc(sizeof(cell_t), cols + 2);
        return prep_board;
    }

    // borders
    // if (rank == 0)
    //     t_lines++;
    // if (rank == size-1)
    //     t_lines++;

    // cell_t** prep_board = malloc(sizeof(cell_t*) * t_lines);

    // int border_offs = 1;  // last section special case...

    // // borders
    // if (rank == 0)
    //     prep_board[0] = calloc(sizeof(cell_t), cols+2);




    // if (rank == size-1)
    //     border_offs = 0;

    // for (int i = 1; i <= lines; i++) {
    //     printf("%d\n", i);
    //     prep_board[i] = calloc(sizeof(cell_t), cols+2);
    //     memcpy(&prep_board[i][1], &s_board[(i-border_offs)*cols], cols);
    // }

    // if (rank == size-1) {
    //     prep_board[0] = calloc(sizeof(cell_t), cols+2);
    //     memcpy(&prep_board[0][1], s_board, cols);
    //     prep_board[t_lines-1] = calloc(sizeof(cell_t), cols+2);
    // }

    // return prep_board;
}

int main(int argc, char *argv[]) {

    MPI_Init(&argc, &argv);

    int c_rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &c_rank);

    int c_size;
    MPI_Comm_size(MPI_COMM_WORLD, &c_size);

    int b_steps;
    int b_size;

    cell_t** m_board;

    // ========================================================================

    // read file
    if (c_rank == 0) {
        FILE* f_board = stdin;
        fscanf(f_board, "%d %d", &b_size, &b_steps);

        m_board = allocate_board(b_size, b_size);

        read_file(f_board, m_board, b_size);

        fclose(f_board);
    }

    MPI_Bcast(&b_steps, 1, MPI_INT, 0, MPI_COMM_WORLD);
    MPI_Bcast(&b_size, 1, MPI_INT, 0, MPI_COMM_WORLD);
    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        if (c_rank == 0)
            printf("\n");
        printf("P%d received b_steps (%d) from root\n", c_rank, b_steps);
    #endif

    // ========================================================================

    // get ranges
    int* b_ranges;
    if (c_rank == 0)
        b_ranges = get_ranges(b_size, c_size);

    int p_range[2];
    MPI_Scatter(b_ranges, 2, MPI_INT, p_range, 2, MPI_INT, 0, MPI_COMM_WORLD);

    if (c_rank == 0)
        free(b_ranges);
    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        if (c_rank == 0)
            printf("\n");
        printf("P%d received b_ranges (%d, %d) from root\n", c_rank, p_range[0], p_range[1]);
    #endif

    // ========================================================================

    // get real ranges
    int* b_real_ranges;
    if (c_rank == 0)
        b_real_ranges = get_real_ranges(b_size, c_size);

    int p_real_ranges[2];
    MPI_Scatter(b_real_ranges, 2, MPI_INT, p_real_ranges, 2, MPI_INT, 0, MPI_COMM_WORLD);

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        if (c_rank == 0)
            printf("\n");
        printf("P%d received p_real_ranges (%d, %d) from root\n", c_rank, p_real_ranges[0], p_real_ranges[1]);
    #endif

    // ========================================================================

    int s_board_offs[c_size];
    int s_board_cnts[c_size];
    cell_t* s_board;
    if (c_rank == 0) {
        s_board = serialize_board(m_board, b_size, b_size);
        for (int i = 0; i < c_size; i++) {
            s_board_offs[i] = b_real_ranges[i*2] * b_size;

            int r_lines = b_real_ranges[i*2+1] - b_real_ranges[i*2];
            s_board_cnts[i] = r_lines * b_size;
        }
    }

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        if (c_rank == 0) {
            printf("\noffsets [");
            for (int i = 0; i < c_size; i++)
                printf(" %d", s_board_offs[i]);
            printf(" ]\n counts [");
            for (int i = 0; i < c_size; i++)
                printf(" %d", s_board_cnts[i]);
            printf(" ]\n");
        }
    #endif

    int p_lines = p_real_ranges[1] - p_real_ranges[0];
    int p_cells = p_lines * b_size;
    cell_t* p_board = malloc(sizeof(cell_t) * p_cells);
    MPI_Scatterv(s_board, s_board_cnts, s_board_offs, MPI_UNSIGNED_CHAR, p_board, p_cells, MPI_UNSIGNED_CHAR, 0, MPI_COMM_WORLD);

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        printf("\n");
        printf("P%d received p_board from root\n", c_rank);
        for (int i = 0; i < p_lines; i++) {
            for (int j = 0; j < b_size; j++)
                printf ("%s", p_board[i*b_size+j] ? "x " : "- ");
            printf("| %d\n", c_rank);
        }
    #endif

    // ========================================================================

    cell_t** p_temp_board = allocate_board(p_lines+2, b_size+2);
    cell_t** p_prep_board = prepare_board(p_board, p_lines, b_size, c_rank, c_size);
    free(p_board);

    #ifdef DEBUG
        MPI_Barrier(MPI_COMM_WORLD);
        printf("\n");
        printf("P%d's board is ready\n", c_rank);
        int extra_lines = 0;
        if (c_rank == 0)
            extra_lines++;
        if (c_rank == c_size-1)
            extra_lines++;

        print_board(p_prep_board, p_lines+extra_lines, b_size+2, c_rank);
    #endif

    // ========================================================================

    // while (b_steps > 0) {

    //     play(p_prep_board, p_temp_board, p_lines, b_size);
    //     printf("P%d finished a step (%d)\n", c_rank, b_steps);
    //     print_board(p_temp_board, p_lines+2, b_size+2, c_rank);
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

    count += board[i-1][j];
    count += board[i-1][j-1];
    count += board[i-1][j+1];
    count += board[i][j-1];
    count += board[i+1][j-1];
    count += board[i+1][j];
    count += board[i+1][j+1];
    count += board[i][j+1];

    return count;
}

void play(cell_t** board, cell_t** newboard, int lines, int cols) {
  int	a;
  /* for each cell, apply the rules of Life */
    for (int i = 1; i <= lines; i++)
        for (int j = 1; j <= cols; j++) {
            a = adjacent_to(board, i, j);
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

void print_board(cell_t** board, int lines, int cols, int rank) {
    for (int j = 0; j < lines; j++) {
        for (int i = 0; i < cols; i++)
            // printf("(%2d, %2d)-", j, i);
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
            board[i][j] = s[i] == 'x';
    }
}
