#include <glib.h>
#include <stdio.h>
#include <service/service.h>
#include "cmd/cmd.h"

int main(int argc, char** argv) {
    // Ensure line buffering
    setvbuf(stdout, (char*)NULL, _IOLBF, 0);
    setvbuf(stderr, (char*)NULL, _IOLBF, 0);

    // Set the program name for later use
    char* cwd = g_get_current_dir();
    char* canon = g_canonicalize_filename(argv[0], cwd);
    g_set_prgname(canon);

    // Parse the command line arguments and call sub-function
    int err = cmd_parse(argc, argv);

    // Cleanup any connection to the gittor seeding service
    gittor_service_disconnect();
    g_free(cwd);
    g_free(canon);

    return err;
}
