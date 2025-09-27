#include <stdlib.h>
#include "calc/calc.h"

#ifdef MOCK_ADD
extern int add(int a, int b) {
    return a + b;
}
#endif
