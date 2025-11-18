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
    bool use_global_scope;
    bool use_local_scope;
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

static error_t parse_opt(int key,
                         __attribute__((__unused__)) char* arg,
                         struct argp_state* state) {
    struct config_arguments* args = state->input;

    switch (key) {
        case 'g':
            args->use_global_scope = true;
            break;

        case 'l':
            args->use_local_scope = true;
            break;

        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_config(struct argp_state* state) {
    // Set defaults arguments
    struct config_arguments args = {.use_global_scope = false,
                                    .use_local_scope = false,
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
    if (err) {
        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;
        return err;
    }

    // Get positional arguments (key and optional value)
    if (argc < 2) {
        fprintf(stderr, "Error: Missing key argument.\n");
        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;
        return ARGP_ERR_UNKNOWN;
    }

    args.key = argv[1];
    if (argc > 2) {
        args.value = argv[2];
    }

    // Parse key into group.key
    char* dot = strchr(args.key, '.');
    if (!dot) {
        fprintf(stderr, "Error: Invalid key format. Use 'group.key'.\n");
        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;
        return ARGP_ERR_UNKNOWN;
    }
    char* group = strndup(args.key, dot - args.key);
    char* key = strdup(dot + 1);

    ConfigScope scope = CONFIG_SCOPE_LOCAL;
    if (args.use_global_scope && !args.use_local_scope) {
        scope = CONFIG_SCOPE_GLOBAL;
    }

    // Read or write the configuration
    if (args.value) {
        // Set configuration
        if (config_set(scope, group, key, args.value) != 0) {
            fprintf(stderr, "Error: Failed to set configuration.\n");
            free(argv[0]);
            argv[0] = argv0;
            state->next += argc - 1;
            return ARGP_ERR_UNKNOWN;
        } else {
            printf("Configuration set: %s.%s = %s\n", group, key, args.value);
        }
    } else {
        // Get configuration
        char* value = config_get(group, key, NULL);
        if (value) {
            printf("%s\n", value);
            free(value);
        } else {
            fprintf(stderr, "Error: Configuration '%s.%s' not found.\n", group,
                    key);
            err = ARGP_ERR_UNKNOWN;
        }
    }

    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;
    if (group) {
        free(group);
    }
    if (key) {
        free(key);
    }
    return err;
}
