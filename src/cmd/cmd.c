#include <argp.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cmd/cmd.h"
#include "config/config.h"
#include "devs/devs.h"
#include "examples/curl.h"
#include "examples/git.h"
#include "examples/ini_parser.h"
#include "examples/tor.h"
#include "init/init.h"
#include "leech/leech.h"
#include "seed/seed.h"
#include "service/service.h"
#include "verify/verify.h"

#define KEY_USAGE 1

static error_t parse_opt(int key, char* arg, struct argp_state* state);

static const char doc[] =
    "COMMANDS:\n"
    "\n"
    "  init     Create an empty GitTor repository\n"
    "  leech    Clone a GitTor repository into a new directory\n"
    "  seed     Share the current state of the repository\n"
    "  devs     Manage who can contribute to this repository\n"
    "  verify   Verify all commits are from authorized developers\n"
    "  config   Get and set GitTor local or global configurations\n"
    "  service  Manage the GitTor service\n"
    "\n"
    "OPTIONS:"
    "\v";

static const struct argp_option options[] = {
    {"path", 'p', "PATH", 0, "The path to the gittor repository", 0},
    {"help", '?', NULL, 0, "Give this help list", -2},
    {"usage", KEY_USAGE, NULL, 0, "Give a short usage message", -1},
    {NULL}};

static struct argp argp = {
    options, parse_opt, "COMMAND [ARGUMENTS...]", doc, NULL, NULL, NULL};

static bool helped = false;
static error_t parse_opt(int key, char* arg, struct argp_state* state) {
    struct global_arguments* args = state->input;

    switch (key) {
        case 'p':
            snprintf(args->path, sizeof(args->path), "%s", arg);
            break;

        case '?':
            argp_help(&argp, stdout, ARGP_HELP_STD_HELP, state->name);
            helped = true;
            break;

        case KEY_USAGE:
            argp_help(&argp, stdout, ARGP_HELP_STD_USAGE, state->name);
            helped = true;
            break;

        case ARGP_KEY_ARG:
            if (strcmp(arg, "init") == 0) {
                return gittor_init(state);
            } else if (strcmp(arg, "leech") == 0) {
                return gittor_leech(state);
            } else if (strcmp(arg, "seed") == 0) {
                return gittor_seed(state);
            } else if (strcmp(arg, "devs") == 0) {
                return gittor_devs(state);
            } else if (strcmp(arg, "verify") == 0) {
                return gittor_verify(state);
            } else if (strcmp(arg, "config") == 0) {
                return gittor_config(state);
            } else if (strcmp(arg, "service") == 0) {
                return gittor_service(state);
            } else if (strcmp(arg, "tor") == 0) {  // TODO(isaac): remove
                torrent_example();
                return 0;
            } else if (strcmp(arg, "ini") == 0) {  // TODO(isaac): remove
                ini_parse();
                return 0;
            } else if (strcmp(arg, "git") == 0) {  // TODO(isaac): remove
                git_example();
                return 0;
            } else if (strcmp(arg, "www") == 0) {  // TODO(isaac): remove
                curl_example();
                return 0;
            } else {
                argp_error(state, "%s is not a valid command", arg);
                return ESRCH;
            }
            break;

        case ARGP_KEY_END:
            if (state->arg_num == 0 && !helped) {
                argp_help(&argp, stdout, ARGP_HELP_STD_USAGE, state->name);
                return EINVAL;
            }
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int cmd_parse(int argc, char** argv) {
    // Defaults
    struct global_arguments args;

    if (getcwd(args.path, sizeof(args.path)) == NULL) {
        perror("getcwd() error");
        return errno;
    }

    // Parse command line arguments
    helped = false;
    return argp_parse(&argp, argc, argv,
                      ARGP_IN_ORDER | ARGP_NO_EXIT | ARGP_NO_HELP, NULL, &args);
}
