#include <glib.h>
#include <stdbool.h>
#include <unistd.h>
#include <gio/gio.h>
#include "service/service.h"
#include "service/service_internals.h"

#ifdef MOCK_GITTOR_SERVICE_RUN
extern int gittor_service_run(bool detached) {
    if (!detached) {
        return gittor_service_main();
    }

    // Notify that the service is not up
    GError* error = NULL;
    gittor_service_set_port(-1, &error);
    if (error) {
        g_printerr("Clear port configuration failed: %s\n", error->message);
        g_clear_error(&error);
        // No need to throw error, things will still work just not as well
    }

    pid_t pid = fork();
    if (pid < 0) {
        g_printerr("Failed to fork: %s\n", g_strerror(errno));
        return -1;
    }

    if (pid == 0) {
        _exit(gittor_service_main());
    }

    signal(SIGCHLD, SIG_IGN);

    // While port is invalid, wait
    int count = 0;
    while (count < 20) {
        int port = gittor_service_get_port(&error);
        if (!error && port > 0) {
            break;
        }
        if (error) {
            g_clear_error(&error);
        }
        g_usleep(100UL * 1000UL);  // 100 ms
        count++;
    }

    return 0;
}
#endif
