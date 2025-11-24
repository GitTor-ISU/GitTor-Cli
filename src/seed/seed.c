#include <git2.h>
#include <glib.h>
#include "seed/seed.h"
#include "seed/seed_internals.h"
#include "service/service.h"

static char* command_to_str(type_e cmd) {
    switch (cmd) {
        case SEED_START:
            return "start";
        case SEED_STOP:
            return "stop";
        case SEED_REMOVE:
            return "remove";
        default:
            return "unknown";
    }
}

static int gittor_seed_command(const git_oid* repo_id, type_e cmd) {
    GError* error = NULL;
    int error_code = 0;
    packet_t msg = {
        .type = cmd, .data = (void*)repo_id, .len = sizeof(*repo_id)};
    packet_t* resp = gittor_service_send(&msg, &error);

    char* repo_id_str = git_oid_tostr_s(repo_id);

    if (error) {
        g_printerr("Failed to %s seeding of repository '%s': %s\n",
                   command_to_str(cmd), repo_id_str, error->message);
        g_clear_error(&error);
        error_code = error->code;
    } else if (resp->type == ERROR) {
        g_printerr("Failed to %s seeding of repository '%s': %s\n",
                   command_to_str(cmd), repo_id_str, (char*)resp->data);
        error_code = 1;
    }
    free(resp->data);
    free(resp);
    return error_code;
}

extern int gittor_seed_start(const git_oid* repo_id) {
    return gittor_seed_command(repo_id, SEED_START);
}

extern int gittor_seed_stop(const git_oid* repo_id) {
    return gittor_seed_command(repo_id, SEED_STOP);
}

extern int gittor_seed_remove(const git_oid* repo_id) {
    return gittor_seed_command(repo_id, SEED_REMOVE);
}
