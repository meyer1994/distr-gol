#include <stdio.h>
#include <stdlib.h>

/**
 * Testing way to convert ints to a char array
 */
int main(int argc, char const *argv[])
{
    int x = 3712;
    char c[10];
    printf("%d\n", x);
    printf("%s\n", c);  // has nothing

    sprintf(c, "%d", x);

    printf("%d\n", x);
    printf("%s\n", c);

    return 0;
}