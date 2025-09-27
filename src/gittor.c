#include <stdio.h>
#include "cmd/cmd.h"

int main(int argc, char** argv) {
    // Ensure line buffering
    setvbuf(stdout, (char*)NULL, _IOLBF, 0);
    setvbuf(stderr, (char*)NULL, _IOLBF, 0);

    // Parse the command line arguments and call sub-function
    return cmd_parse(argc, argv);
}
