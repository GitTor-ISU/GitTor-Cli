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
#include "utils/utils.h"

#define KEY_USAGE 1

struct leech_arguments {
    struct global_arguments* global;
    char* key;
    key_type_e type;
    char* destination;
};

static error_t parse_opt(int key, char* arg, struct argp_state* state);
static key_type_e key_type(const char* key);
static int get_repo_id(char* str, size_t n, const char* pat);
static bool remote_url_matches_path(const char* remote_url, const char* path);
static int infer_clone_destination(const char* global_path,
                                   const char* repo_id,
                                   char* out,
                                   size_t out_size);

static struct argp_option options[] = {
    {"help", '?', NULL, 0, "Give this help list", -2},
    {"usage", KEY_USAGE, NULL, 0, "Give a short usage message", -1},
    {NULL, 0, NULL, 0, NULL, 0}};

static char doc[] = "Downloads a repository given its key.";

static struct argp argp = {options, parse_opt, "[KEY] [DIRECTORY]", doc, NULL,
                           NULL,    NULL};

static char inferred_repo_id[GIT_OID_HEXSZ + 1];

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
            } else if (state->arg_num == 1) {
                args->destination = arg;
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
                int err =
                    get_repo_id(inferred_repo_id, sizeof(inferred_repo_id),
                                args->global->path);
                if (err < 0) {
                    const git_error* e = git_error_last();
                    if (e) {
                        printf("Error %d/%d: %s\n", err, e->klass, e->message);
                    }
                }

                args->key = inferred_repo_id;
                args->type = REPO_ID;
                args->destination = args->global->path;
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
    char leeched_path[PATH_MAX] = {0};
    helped = false;
    int err = 0;
    int ret = -1;
    git_repository* leeched_repo = NULL;
    git_repository* destination_repo = NULL;
    git_remote* origin = NULL;
    char leeched_repo_id[GIT_OID_HEXSZ + 1] = {0};
    char inferred_destination[PATH_MAX] = {0};
    const char* destination = NULL;

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
    err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);
    if (err || helped) {
        goto end;
    }

    // Stub output for template
    err = leech_repository(args.key, args.type, leeched_path,
                           sizeof(leeched_path));
    if (err) {
        goto end;
    }

    // Initialize libgit2
    ret = git_libgit2_init();
    if (ret < 0) {
        err = ret;
        goto end;
    }

    // Open the leeched bare repository
    err = git_repository_open(&leeched_repo, leeched_path);
    if (err) {
        goto end;
    }

    // Get repository ID
    err = gittor_get_repo_id(leeched_repo_id, sizeof(leeched_repo_id),
                             leeched_repo);
    if (err) {
        goto end;
    }

    // Evaluate the proper destination
    destination = args.destination;
    if (destination == NULL) {
        err = infer_clone_destination(args.global->path, leeched_repo_id,
                                      inferred_destination,
                                      sizeof(inferred_destination));
        if (err) {
            goto end;
        }
        destination = inferred_destination;
    }

    // If destination already points to this same remote repository and
    // repository IDs match, fetch and tell the user to pull. Otherwise, clone
    // into the destination directory.
    if (!git_repository_open(&destination_repo, destination)) {
        char destination_repo_id[GIT_OID_HEXSZ + 1] = {0};
        err = gittor_get_repo_id(destination_repo_id,
                                 sizeof(destination_repo_id), destination_repo);
        if (err) {
            goto end;
        }

        err = git_remote_lookup(&origin, destination_repo, "origin");
        if (err) {
            goto end;
        }

        if (strcmp(destination_repo_id, leeched_repo_id) == 0 &&
            remote_url_matches_path(git_remote_url(origin), leeched_path)) {
            git_fetch_options fetch_opts;
            err =
                git_fetch_options_init(&fetch_opts, GIT_FETCH_OPTIONS_VERSION);
            if (err) {
                goto end;
            }

            err = git_remote_fetch(origin, NULL, &fetch_opts, NULL);
            if (err) {
                goto end;
            }

            printf(
                "Fetched latest changes. Run `git pull` in '%s' to apply the "
                "latest changes\n",
                destination);
        } else {
            git_repository_free(destination_repo);
            destination_repo = NULL;
            err = git_clone(&destination_repo, leeched_path, destination, NULL);
            if (err) {
                goto end;
            }
        }
    } else {
        err = git_clone(&destination_repo, leeched_path, destination, NULL);
        if (err) {
            goto end;
        }
    }

end:
    // Error handling and clean up
    if (err < 0) {
        const git_error* e = git_error_last();
        if (e) {
            printf("Error %d/%d: %s\n", err, e->klass, e->message);
        }
    }
    if (!ret) {
        git_libgit2_shutdown();
    }
    git_remote_free(origin);
    git_repository_free(leeched_repo);
    git_repository_free(destination_repo);

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

static int get_repo_id(char* str, size_t n, const char* path) {
    int err = 0;
    int ret = -1;
    git_repository* repo = NULL;

    // Initialize libgit2
    ret = git_libgit2_init();
    if (ret < 0) {
        err = ret;
    }

    // Open the local repository
    if (!err) {
        err = git_repository_open(&repo, path);
    }

    // Get the repo ID
    if (!err) {
        err = gittor_get_repo_id(str, n, repo);
    }

    if (ret > 0) {
        git_libgit2_shutdown();
    }
    git_repository_free(repo);

    return err;
}

static bool remote_url_matches_path(const char* remote_url, const char* path) {
    if (remote_url == NULL || path == NULL) {
        return false;
    }

    if (strcmp(remote_url, path) == 0) {
        return true;
    }

    const char prefix[] = "file://";
    size_t prefix_len = strlen(prefix);
    if (strncmp(remote_url, prefix, prefix_len) == 0 &&
        strcmp(remote_url + prefix_len, path) == 0) {
        return true;
    }

    return false;
}

static int infer_clone_destination(const char* global_path,
                                   const char* repo_id,
                                   char* out,
                                   size_t out_size) {
    if (global_path == NULL || repo_id == NULL || out == NULL ||
        out_size == 0) {
        return EINVAL;
    }

    size_t base_len = strlen(global_path);
    size_t repo_len = strlen(repo_id);
    bool needs_sep = (base_len > 0 && global_path[base_len - 1] != '/');
    size_t total_len = base_len + (needs_sep ? 1 : 0) + repo_len + 1;
    if (total_len > out_size) {
        return ENAMETOOLONG;
    }

    g_snprintf(out, out_size, "%s%s%s", global_path, needs_sep ? "/" : "",
               repo_id);
    return 0;
}
