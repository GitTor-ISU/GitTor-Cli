#include <glib.h>
#include <stdio.h>
#include "cmd/cmd.h"

int main(int argc, char** argv) {
    // Ensure line buffering
    setvbuf(stdout, (char*)NULL, _IOLBF, 0);
    setvbuf(stderr, (char*)NULL, _IOLBF, 0);

    // Set the program name for later use
    g_set_prgname(g_canonicalize_filename(argv[0], g_get_current_dir()));

    // Parse the command line arguments and call sub-function
    return cmd_parse(argc, argv);
}
