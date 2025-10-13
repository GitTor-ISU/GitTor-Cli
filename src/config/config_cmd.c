#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "config/config.h"

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct config_arguments {
    struct global_arguments* global;
    bool global_scope;
    bool local_scope;
    char* key;
    char* value;
};

static struct argp_option options[] = {
    {"global", 'g', 0, 0, "Use global (user-wide) configuration", 0},
    {"local", 'l', 0, 0, "Use local (repository-wide) configuration", 0},
    {NULL}};

static char doc[] =
    "Read and write global (user-wide) or local "
    "(repository-wide) configurations.";

static struct argp argp = {options, parse_opt, "<key> [<value>]", doc, NULL,
                           NULL,    NULL};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    // Unused parameter marked to prevent linter warnings
    (void)arg;

    struct config_arguments* args = state->input;

    switch (key) {
        case 'g':
            args->global_scope = true;
            break;

        case 'l':
            args->local_scope = true;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_config(struct argp_state* state) {
    // Set defaults arguments
    struct config_arguments args = {.global_scope = false,
                                    .local_scope = false,
                                    .key = NULL,
                                    .value = NULL};

    // Change the arguments array for just config
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor config
    const char name[] = "config";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    // Stub output for template
    printf("%s PATH: %s (config command not yet implemented)\n", argv[0],
           args.global->path);

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}
