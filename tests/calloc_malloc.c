#include <stdlib.h>

int main(int argc, char const *argv[])
{
    for (int i = 0; i < 10000000; i++) {
        int* x = calloc(sizeof(int), 5000);
        // int* x = malloc(sizeof(int) * 5000);
        free(x);
    }
    return 0;
}