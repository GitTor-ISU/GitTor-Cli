#include <math.h>
#include <stdio.h>
#include "calc/calc.h"

int main(int argc __attribute__((unused)),
         char** argv __attribute__((unused))) {
    printf("Hello, World\n");
    printf("1 + 2 = %d\n", add(1, 2));
    printf("pi = %f\n", M_PI);
    return 0;
}
