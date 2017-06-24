#include <stdio.h>
#include <mpi.h>

int main(int argc, char const *argv[])
{

    MPI_Init(&argc, &argv);

    int rank;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);


    if (rank == 0) {

    } else {

    }


    MPI_Finalize();
    return 0;
}
