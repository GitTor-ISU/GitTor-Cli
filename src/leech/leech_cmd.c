#include <argp.h>
#include <assert.h>
#include <ctype.h>
#include <git2.h>
#include <glib.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cmd/cmd.h"
#include "leech/leech.h"
#include "leech/leech_internal.h"

#define KEY_USAGE 1

struct leech_arguments {
    struct global_arguments* global;
    char* key;
    key_type_e type;
};

static error_t parse_opt(int key, char* arg, struct argp_state* state);
static key_type_e key_type(const char* key);

static struct argp_option options[] = {
    {"help", '?', NULL, 0, "Give this help list", -2},
    {"usage", KEY_USAGE, NULL, 0, "Give a short usage message", -1},
    {NULL}};

static char doc[] = "Downloads a repository given its key.";

static struct argp argp = {options, parse_opt, "<KEY>", doc, NULL, NULL, NULL};

static bool helped;
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct leech_arguments* args = state->input;

    switch (key) {
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) {
                key_type_e type = key_type(arg);
                if (type == INVALID) {
                    argp_error(
                        state,
                        "Invalid KEY, '%s' must be a 40-character hex "
                        "string, path to a .torrent file, or magnet link.",
                        args->key);
                    return EINVAL;
                }
                args->key = arg;
                args->type = type;
            } else {
                return E2BIG;
            }
            break;
        case '?':
            argp_help(&argp, stdout, ARGP_HELP_STD_HELP, state->name);
            helped = true;
            break;
        case ARGP_KEY_END:
            if (state->arg_num == 0 && !helped) {
                argp_help(&argp, stdout, ARGP_HELP_STD_USAGE, state->name);
                return EINVAL;
            }
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

extern int gittor_leech(struct argp_state* state) {
    // Set defaults arguments
    struct leech_arguments args = {0};
    helped = false;

    // Change the arguments array for just leech
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor leech
    const char name[] = "leech";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    g_snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);

    // Stub output for template
    if (!err) {
        leech_repository(args.key, args.type);
        printf("leeched!\n");
    }

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}

static key_type_e key_type(const char* key) {
    if (key == NULL) {
        return INVALID;
    }

    size_t len = g_utf8_strlen(key, PATH_MAX);

    // Check if it's a 40 character hex string repository key
    if (len == GIT_OID_HEXSZ) {
        bool all_hex = true;
        for (size_t i = 0; i < len; i++) {
            if (!isxdigit(key[i])) {
                all_hex = false;
                break;
            }
        }
        if (all_hex) {
            return REPO_ID;
        }
    }

    // Check if it's a magnet link
    if (strncmp(key, "magnet:?", 8) == 0) {
        return MAGNET_LINK;
    }

    // Check if it's a .torrent file path
    if (len > 8 && strcmp(key + len - 8, ".torrent") == 0) {
        return TORRENT_PATH;
    }

    return INVALID;
}
