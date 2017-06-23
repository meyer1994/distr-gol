typedef unsigned char cell_t;

cell_t** allocate_board(int lines, int cols) {
    cell_t** board = (cell_t**) malloc(sizeof(cell_t*) * lines);
    for (int i = 0; i < lines; i++)
        board[i] = (cell_t *) malloc(sizeof(cell_t) * cols);
    return board;
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
            printf("%s", board[j][i] ? "x " : "- ");
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
cell_t** get_sending_regions(cell_t** board, int lines, int cols, int regions) {
    cell_t** boards = malloc(sizeof(cell_t*) * regions);
    int** dims = get_sending_region_dimensions(lines, cols, regions);

    int region_start = 0;

    // top region (+1) for the top border of this region
    int first_region_cells = dims[0][0] * dims[0][1];
    boards[0] = malloc(sizeof(cell_t) * first_region_cells);
    for (int i = 0; i < dims[0][0]; i++)
        for (int j = 0; j < dims[0][1]; j++)
            boards[0][i * dims[0][1] + j] = board[i][j];
    region_start += dims[0][0]-1;


    for (int i = 1; i < regions - 1; i++) {
        int region_cells = dims[i][0] * dims[i][1];
        boards[i] = malloc(sizeof(cell_t) * region_cells);

        for (int j = 0; j < dims[i][0]; j++)
            for (int k = 0; k < dims[i][1]; k++)
                boards[i][(j * dims[i][1]) + k] = board[region_start+j-1][k];

        region_start += dims[i][0]-1;
    }

    int last_i = regions-1;
    int last_region_cells = dims[last_i][0] * dims[last_i][1];
    boards[last_i] = malloc(sizeof(cell_t) * last_region_cells);
    for (int i = 0; i < dims[last_i][0]; i++) {
        for (int j = 0; j < dims[last_i][1]; j++)
        // very ugly coordinates...
        boards[last_i][(i * dims[0][1]) + j] = board[lines-dims[last_i][0]+i][j];
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