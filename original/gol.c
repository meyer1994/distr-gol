#include <stdio.h>
#include <stdlib.h>
typedef unsigned char cell_t;


// prototypes
cell_t** allocate_board(int size);
void free_board(cell_t** board, int size);
inline int adjacent_to(cell_t** board, int size, int i, int j);
void play(cell_t** board, cell_t** newboard, int size);
void print(cell_t** board, int size);
void read_file(FILE* f, cell_t** board, int size);

int main () {
  int size, steps;
  FILE* f;
  f = stdin;
  fscanf(f, "%d %d", &size, &steps);
  cell_t** prev = allocate_board(size);
  read_file(f, prev,size);
  fclose(f);
  cell_t** next = allocate_board(size);
  cell_t** tmp;

  #ifdef DEBUG
    printf("Initial:\n");
    print(prev, size);
  #endif

  for (int i = 0; i < steps; i++) {
    play(prev, next, size);
    #ifdef DEBUG
      printf("%d ----------\n", i + 1);
      print(next, size);
    #endif
    tmp = next;
    next = prev;
    prev = tmp;
  }

  #ifdef RESULT
    printf("Final:\n");
    print (prev,size);
  #endif

  free_board(prev,size);
  free_board(next,size);
}

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

/* return the number of on cells adjacent to the i,j cell */
inline int adjacent_to(cell_t** board, int size, int i, int j) {
  int count = 0;
  if (i-1 > 0) {
    count += board[i-1][j];
    if (j - 1 > 0)
      count += board[i-1][j-1];
    if (j+1 < size)
      count += board[i-1][j+1];
  }

  if (j-1 > 0) {
    count += board[i][j-1];
    if (i+1 < size)
      count += board[i+1][j-1];
  }

  if (i+1 < size) {
    count += board[i+1][j];
    if (j+1 < size)
      count += board[i+1][j+1];
  }

  if (j+1 < size)
    count += board[i][j+1];
  return count;
}

void play(cell_t** board, cell_t** newboard, int size) {
  int	a;
  /* for each cell, apply the rules of Life */
  for (int i = 0; i < size; i++)
    for (int j = 0; j < size; j++) {
      a = adjacent_to(board, size, i, j);
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
