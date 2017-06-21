#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/**
 * Test created to see if a normal MPI_Recv could receive info from a MPI_Isend
 * IT CAN!
 */
int main(int argc, char *argv[])
{

    MPI_Init(&argc, &argv);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    if (rank == 0) {

        MPI_Request reqs[size-1];
        for (int i = 1; i < size; i++) {
            int x = 11 * i;
            MPI_Isend(&x, 1,  MPI_INT, i, 0, MPI_COMM_WORLD, &reqs[i-1]);
        }

        printf("WAIT!\n");
        MPI_Status st;
        for (int i = 0; i < size-1; i++)
            MPI_Wait(&reqs[i], &st);

        printf("This will print after everything\n");



    } else {
        int x;
        MPI_Status st;
        MPI_Recv(&x, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);

        printf("Child %d received %d\n", rank, x);

    }

    MPI_Finalize();
}