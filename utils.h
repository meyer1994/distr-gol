#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/**
 * @brief Custom cell type.
 */
typedef unsigned char cell_t;


// prototypes
cell_t** allocate_board(int size);
void free_board(cell_t** board, int size);
inline int adjacent_to(cell_t** board, int size, int i, int j);
void play(cell_t** board, cell_t** newboard, int size);
void print(cell_t** board, int size);
void read_file(FILE* f, cell_t** board, int size);

cell_t** allocate_board(int size) {
  cell_t** board = (cell_t**) malloc(sizeof(cell_t*) * size);
  for (int i = 0; i < size; i++)
    board[i] = (cell_t *) malloc(sizeof(cell_t) * size);
  return board;
}

void free_board(cell_t** board, int size) {
  for (int i = 0; i < size; i++)
    free(board[i]);
  free(board);
}

/* print the life board */
void print(cell_t** board, int size) {
  /* for each row */
  for (int j = 0; j < size; j++) {
    /* print each column position... */
    for (int i = 0; i < size; i++)
      printf ("%c", board[i][j] ? 'x' : ' ');
    /* followed by a carriage return */
    printf ("\n");
  }
}

/* read a file into the life board */
void read_file(FILE* f, cell_t** board, int size) {
  char* s = (char*) malloc(size + 10);
  /* read the first new line (it will be ignored) */
  fgets(s, size + 10, f);

  /* read the life board */
  for (int j = 0; j < size; j++) {
    /* get a string */
    fgets (s, size + 10, f);
    /* copy the string to the life board */
    for (int i = 0; i < size; i++)
      board[i][j] = s[i] == 'x';
  }
}
