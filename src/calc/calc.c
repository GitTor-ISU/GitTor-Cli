#include "calc/calc.h"
#include <stdlib.h>

#ifndef MOCK_ADD
extern int add(int a, int b) {
    return a - b;
}
#endif
