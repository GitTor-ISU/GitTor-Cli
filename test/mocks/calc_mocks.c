#include <stdlib.h>
#include "examples/calc.h"

#ifdef MOCK_ADD
extern int add(int a, int b) {
    return a + b;
}
#endif
