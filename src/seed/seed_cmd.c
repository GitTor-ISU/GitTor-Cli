#include <assert.h>
#include <git2.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "seed/seed.h"
#include "seed/seed_internals.h"
#include "utils/utils.h"

#define KEY_USAGE 1

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct seed_arguments {
    struct global_arguments* global;
};

static struct argp_option options[] = {
    {"help", '?', NULL, 0, "Give this help list", -2},
    {"usage", KEY_USAGE, NULL, 0, "Give a short usage message", -1},
    {NULL}};

static char doc[] = "Uploads and shares the current state of the repository.";

struct argp argp = {options, parse_opt, "", doc, NULL, NULL, NULL};

static bool helped;
static error_t parse_opt(int key,
                         __attribute__((__unused__)) char* arg,
                         __attribute__((__unused__)) struct argp_state* state) {
    switch (key) {
        case '?':
            argp_help(&argp, stdout, ARGP_HELP_STD_HELP, state->name);
            helped = true;
            break;

        case KEY_USAGE:
            argp_help(&argp, stdout, ARGP_HELP_STD_USAGE, state->name);
            helped = true;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_seed(struct argp_state* state) {
    // Set defaults arguments
    struct seed_arguments args = {0};
    helped = false;

    // Change the arguments array for just seed
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor seed
    const char name[] = "seed";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    g_snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    // Initialize libgit2
    if (!err && !helped) {
        int ret = git_libgit2_init();
        if (ret < 0) {
            err = ret;
        }
    }

    // Open the local repository
    git_repository* repo = NULL;
    if (!err && !helped) {
        err = git_repository_open(&repo, args.global->path);
    }

    // Get the repo ID
    git_oid repo_id;
    if (!err && !helped) {
        gittor_get_repo_id(&repo_id, repo);
    }

    // Tell the seeder service to stop seeding this
    if (!err && !helped) {
        err = gittor_seed_stop(&repo_id);
    }

    // Push any new changes to the repo
    if (!err && !helped) {
        err = gittor_git_push(repo);
    }

    // Tell the seeder service to begin seeding this
    if (!err && !helped) {
        err = gittor_seed_start(&repo_id);
    }

    if (err < 0) {
        const git_error* e = git_error_last();
        if (e) {
            printf("Error %d/%d: %s\n", err, e->klass, e->message);
        }
    }

    // Reset back to global
    git_repository_free(repo);
    git_libgit2_shutdown();
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}
