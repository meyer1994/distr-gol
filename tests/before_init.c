#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

/**
 * Teste created to see if the things before the MPI_Init are run aswell.
 * THEY ARE.
 */
int main(int argc, char *argv[])
{
    printf("Before INIT!\n");

    MPI_Init(&argc, &argv);

    printf("After INIT\n");

    MPI_Finalize();
}