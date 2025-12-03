#include "examples/calc.h"

#ifndef MOCK_ADD
extern int add(int a, int b) {
    return a - b;
}
#endif
