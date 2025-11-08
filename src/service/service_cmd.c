#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "service/service.h"

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct service_arguments {
    struct global_arguments* global;
};

static struct argp_option options[] = {{NULL}};

static const char doc[] =
    "COMMANDS:\n"
    "\n"
    "  start    Ensures the GitTor service is running\n"
    "  stop     Ensures the GitTor service is not running\n"
    "  restart  Stops and starts the GitTor service\n"
    "\n"
    "OPTIONS:"
    "\v";

static struct argp argp = {options, parse_opt, NULL, doc, NULL, NULL, NULL};

static error_t parse_opt(int key,
                         char* arg,
                         __attribute__((__unused__)) struct argp_state* state) {
    switch (key) {
        case ARGP_KEY_ARG:
            if (strcmp(arg, "start") == 0) {
                return gittor_service_start();
            } else if (strcmp(arg, "stop") == 0) {
                return gittor_service_stop();
            } else if (strcmp(arg, "restart") == 0) {
                return gittor_service_restart();
            } else if (strcmp(arg, "ping") == 0) {
                return gittor_service_ping();
            } else if (strcmp(arg, "run") == 0) {  // Hidden command
                return gittor_service_main();
            } else {
                argp_error(state, "%s is not a valid command", arg);
                return ESRCH;
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_service(struct argp_state* state) {
    // Set defaults arguments
    struct service_arguments args = {0};

    // Change the arguments array for just service
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor service
    const char name[] = "service";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}
