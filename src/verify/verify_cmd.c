#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "verify/verify.h"

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct verify_arguments {
    struct global_arguments* global;
    bool branch;
};

static struct argp_option options[] = {
    {"branch", 'b', 0, 0, "Verify commits for a specific branch", 0},
    {NULL}
};

static char doc[] =
    "Verifies that all commits on the current branch are "
    "signed by authorized developers.";

static struct argp argp = {options, parse_opt, "", doc, NULL, NULL, NULL};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct verify_arguments* args = state->input;

    switch (key) {
        case 'b':
            args->branch = true;
            break;
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_verify(struct argp_state* state) {
    // Set defaults arguments
    struct verify_arguments args = { 0 };

    // Change the arguments array for just verify
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor verify
    const char name[] = "verify";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    // Stub output for template
    printf("%s PATH: %s (verify command not yet implemented)\n",
           argv[0], args.global->path);

    // Rest back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}
