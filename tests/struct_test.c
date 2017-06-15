#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

typedef struct test_s {
    int x;
    int y;
} test_s;

int* serialize(test_s s) {
    int* ser = malloc(sizeof(int) * 2);
    ser[0] = s.x;
    ser[1] = s.y;
    return ser;
}

test_s deserialize(int* s) {
    test_s x;
    x.x = s[0];
    x.y = s[1];

    return x;
}

/**
 * Testing if I can use an struct in my slave processes. I can. Not tested on a
 * network. Only locally.
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
            test_s t;
            t.x = i;
            t.y = i + 10;
            int* x = serialize(t);
            MPI_Send(x, 2, MPI_INT, i, 0, MPI_COMM_WORLD);
            free(x);
        }


    } else {
        int* recveid;
        MPI_Status st;
        MPI_Recv(recveid, 2, MPI_INT, 0, 0, MPI_COMM_WORLD, &st);

        printf("TESTE!\n");

        test_s s = deserialize(recveid);

        printf("rank %d\n%d\t%d\n", rank, s.x, s.y);
        // printf("rank %d\n%d\t%d\n", rank, recveid[0], recveid[1]);



    }

    MPI_Finalize();

    return 0;
}