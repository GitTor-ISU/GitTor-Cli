#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "devs/devs.h"

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct devs_arguments {
    struct global_arguments* global;
};

static struct argp_option options[] = {{NULL}};

static char doc[] = "Manage who can contribute to this repository.";

static struct argp argp = {options, parse_opt, "", doc, NULL, NULL, NULL};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    // Unused parameters marked to prevent linter warnings
    (void)arg;
    (void)state;

    switch (key) {
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_devs(struct argp_state* state) {
    // Set defaults for devs arguments
    struct devs_arguments args = {0};

    // Prepare arguments array for just devs
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor devs
    const char name[] = "devs";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    // Stub output for template
    printf("%s PATH: %s (devs command not yet implemented)\n", argv[0],
           args.global->path);

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}
