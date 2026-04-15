#include <assert.h>
#include <git2.h>
#include <glib.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "api/torrents.h"
#include "cmd/cmd.h"
#include "seed/seed.h"
#include "seed/seed_internals.h"
#include "utils/utils.h"

#define KEY_USAGE 1

static error_t parse_opt(int key, char* arg, struct argp_state* state);
static int get_torrent_name(char* out_name, size_t out_size);
static int get_torrent_description(char* out_description, size_t out_size);

struct seed_arguments {
    struct global_arguments* global;
};

static struct argp_option options[] = {
    {"help", '?', NULL, 0, "Give this help list", -2},
    {"usage", KEY_USAGE, NULL, 0, "Give a short usage message", -1},
    {NULL}};

static char doc[] = "Uploads and shares the current state of the repository.";

static struct argp argp = {options, parse_opt, "", doc, NULL, NULL, NULL};

static bool helped;

static int get_torrent_name(char* out_name, size_t out_size) {
    if (!out_name || out_size == 0) {
        return -1;
    }

    gint read_size = (out_size > G_MAXINT) ? G_MAXINT : (gint)out_size;
    while (true) {
        g_print("Torrent name: ");
        if (!fgets(out_name, read_size, stdin)) {
            return -1;
        }

        if (!strchr(out_name, '\n')) {
            int c;
            while ((c = getchar()) != '\n' && c != EOF) {
            }
        }

        g_strchomp(out_name);
        if (out_name[0] != '\0') {
            return 0;
        }

        g_print("Torrent name cannot be empty.\n");
    }
}

static int get_torrent_description(char* out_description, size_t out_size) {
    if (!out_description || out_size == 0) {
        return -1;
    }

    gint read_size = (out_size > G_MAXINT) ? G_MAXINT : (gint)out_size;
    g_print("Torrent description (optional): ");
    if (!fgets(out_description, read_size, stdin)) {
        return -1;
    }

    if (!strchr(out_description, '\n')) {
        int c;
        while ((c = getchar()) != '\n' && c != EOF) {
        }
    }

    g_strchomp(out_description);
    return 0;
}

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
    int err = 0;
    int ret = -1;
    api_result_e result = API_OK;
    git_repository* repo = NULL;
    torrent_dto_t* torrent_dto = NULL;
    char torrent_name[256] = {0};
    char torrent_description[512] = {0};

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
    err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);
    if (err || helped) {
        goto end;
    }

    // Initialize libgit2
    ret = git_libgit2_init();
    if (ret < 0) {
        goto end;
    }

    // Open the local repository
    err = git_repository_open(&repo, args.global->path);
    if (err) {
        goto end;
    }

    // Get the repo ID
    git_oid repo_id;
    err = gittor_get_repo_id(&repo_id, repo);
    if (err) {
        goto end;
    }
    char repo_id_str[GIT_OID_HEXSZ + 1];
    git_oid_tostr(repo_id_str, sizeof(repo_id_str), &repo_id);

    // Tell the seeder service to stop seeding this
    err = gittor_seed_stop(&repo_id);
    if (err) {
        goto end;
    }

    // Push any new changes to the repo
    err = gittor_git_push(repo);
    if (err) {
        goto end;
    }

    // Tell the seeder service to begin seeding this
    err = gittor_seed_start(&repo_id);
    if (err) {
        goto end;
    }

    // Get the path to the torrent file
    char torrent_file_path[PATH_MAX];
    err = gittor_remote_path(torrent_file_path, &repo_id);
    if (err) {
        goto end;
    }
    g_strlcat(torrent_file_path, ".torrent", sizeof(torrent_file_path));

    // Check if repository exists in web
    torrent_dto = api_get_torrent_by_repo_id(repo_id_str, &result);
    if (result == API_OK) {
        err = api_update_torrent_file(torrent_dto->id, torrent_file_path,
                                      &result);
        if (err) {
            goto end;
        }
    } else if (result == API_NOT_FOUND) {
        err = get_torrent_name(torrent_name, sizeof(torrent_name));
        if (err) {
            goto end;
        }

        err = get_torrent_description(torrent_description,
                                      sizeof(torrent_description));
        if (err) {
            goto end;
        }

        torrent_upload_t upload = {
            .name = torrent_name,
            .description =
                (torrent_description[0] != '\0') ? torrent_description : NULL,
            .repo_id = repo_id_str,
            .file_path = torrent_file_path};
        torrent_dto = api_upload_torrent(&upload, &result);
    } else {
        err = result;
        g_printerr("Error (%d): Failed to communicate with API.\n", err);
    }

    printf("Seeding Repository %s!\n", repo_id_str);

end:
    if (err < 0) {
        const git_error* e = git_error_last();
        if (e) {
            printf("Error %d/%d: %s\n", err, e->klass, e->message);
        }
    }
    if (!ret) {
        git_libgit2_shutdown();
    }
    git_repository_free(repo);
    torrent_dto_free(torrent_dto);

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}
