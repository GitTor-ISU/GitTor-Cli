#include <git2.h>
#include <glib.h>
#include "seed/seed.h"
#include "service/service.h"

static char* command_to_str(type_e cmd) {
    switch (cmd) {
        case SEED_START:
            return "start";
        case SEED_STOP:
            return "stop";
        default:
            return "unknown";
    }
}

static int gittor_seed_command(const char* repo_id, type_e cmd) {
    GError* error = NULL;
    int error_code = 0;
    packet_t msg = {.type = cmd,
                    .data = (void*)repo_id,
                    .len = (ssize_t)(strlen(repo_id) + 1)};
    packet_t* resp = gittor_service_send(&msg, &error);

    if (error) {
        g_printerr("Failed to %s seeding of repository '%s': %s\n",
                   command_to_str(cmd), repo_id, error->message);
        error_code = error->code;
        g_clear_error(&error);
    } else if (resp && resp->type == SERVICE_ERROR) {
        g_printerr("Failed to %s seeding of repository '%s': %s\n",
                   command_to_str(cmd), repo_id, (char*)resp->data);
        error_code = 1;
    } else if (!resp) {
        g_printerr(
            "Failed to %s seeding of repository '%s': no response from "
            "service\n",
            command_to_str(cmd), repo_id);
        error_code = 1;
    }

    if (resp) {
        free(resp->data);
        free(resp);
    }

    return error_code;
}

extern int gittor_seed_start(const char* repo_id) {
    return gittor_seed_command(repo_id, SEED_START);
}

extern int gittor_seed_stop(const char* repo_id) {
    return gittor_seed_command(repo_id, SEED_STOP);
}
