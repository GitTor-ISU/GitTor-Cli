#include <assert.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "init/init.h"

#define KEY_USAGE 1

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct init_arguments {
    struct global_arguments* global;
    char* dir;
};

static struct argp_option options[] = {
    {"help", '?', NULL, 0, "Give this help list", -2},
    {"usage", KEY_USAGE, NULL, 0, "Give a short usage message", -1},
    {NULL}};

static char doc[] =
    "Initializes a new GitTor repository in the current directory.";

static struct argp argp = {options, parse_opt, "[DIRECTORY]", doc,
                           NULL,    NULL,      NULL};

static bool helped;
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct init_arguments* args = state->input;
    size_t arglen;

    switch (key) {
        case '?':
            argp_help(&argp, stdout, ARGP_HELP_STD_HELP, state->name);
            helped = true;
            break;

        case KEY_USAGE:
            argp_help(&argp, stdout, ARGP_HELP_STD_USAGE, state->name);
            helped = true;
            break;

        case ARGP_KEY_ARG:
            arglen = strlen(arg);
            args->dir = malloc(arglen + 1);
            g_snprintf(args->dir, arglen + 1, "%s", arg);
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_init(struct argp_state* state) {
    // Set defaults arguments
    struct init_arguments args = {.dir = NULL};
    helped = false;

    // Change the arguments array for just init
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor init
    const char name[] = "init";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    g_snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    // Create bare repository
    char bare_repo[FILE_URL_MAX];
    if (!err && !helped) {
        err = create_bare_repo(bare_repo);
    }

    // Clone bare repository
    gchar* path = g_build_filename(args.global->path, args.dir, NULL);
    if (!err && !helped) {
        err = clone_bare_repo(bare_repo, path);
    }
    free(path);

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    if (args.dir) {
        free(args.dir);
    }
    return err;
}
