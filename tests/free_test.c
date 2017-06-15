#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>

/**
 * This test was made to see if freeing some value after sending it would have
 * any effect in the execution. I thought not and, indeed, it does not. But it
 * was more just to be sure.
 */
int main(int argc, char *argv[])
{

    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (rank == 0) {
        for (int i = 1; i < size; i++) {
            int* x = malloc(sizeof(int));
            *x = i;

            MPI_Send(x, 1, MPI_INT, i, 0, MPI_COMM_WORLD);

            // even if the value is freed, it was done after it was sent.
            free(x);
        }

    } else {
        int value;
        MPI_Status stat;
        MPI_Recv(&value, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &stat);

        printf("Rank %d, received %d\n", rank, value);

    }


    MPI_Finalize();

    return 0;
}