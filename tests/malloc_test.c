#include <stdio.h>
#include <stdlib.h>


/**
 * Test created to see if you could pass a pointer to an array without allocating
 * it beforehand. You can't. Need to allocate before passing it to a function.
 */
int print(int** x, int size) {
    for (int i = 0; i < 10; i++) {
        for (int j = 0; j < 10; j++)
            printf("%d ", x[i][j]);
        printf("\n");
    }
}

int main(int argc, char const *argv[])
{
    int** x;
    x = malloc(sizeof(int*) * 10);
    for (int i = 0; i < 10; i++)
        x[i] = malloc(sizeof(int) * 10);

    for (int i = 0; i < 10; i++)
        for (int j = 0; j < 10; j++)
            x[i][j] = 1;

    print(x, 10);

    return 0;
}