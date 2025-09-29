#include <assert.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "init/init.h"

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct init_arguments {
    struct global_arguments* global;
    char* name;
};

static struct argp_option options[] = {
    {"name", 'n', "NAME", 0, "The name of the repository", 0},
    {NULL}};

static char doc[] =
    "Initializes a new GitTor repository in the current directory.";

static struct argp argp = {options, parse_opt, "REPOSITORY", doc,
                           NULL,    NULL,      NULL};

static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct init_arguments* args = state->input;
    size_t arglen;

    switch (key) {
        case 'n':
            arglen = strlen(arg);
            args->name = malloc(arglen + 1);
            snprintf(args->name, arglen + 1, "%s", arg);
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_init(struct argp_state* state) {
    // Set defaults arguments
    struct init_arguments args = {.name = NULL};

    // Change the arguments array for just init
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor init
    const char name[] = "init";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    if (args.name) {
        printf("%s PATH: %s NAME: %s\n", argv[0], args.global->path, args.name);
    } else {
        printf("%s PATH: %s\n", argv[0], args.global->path);
    }

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    if (args.name) {
        free(args.name);
    }
    return err;
}
