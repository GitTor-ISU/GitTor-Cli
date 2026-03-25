#include <glib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>
#include "api/internal.h"
#include "config/config.h"
#include "login/login.h"

static error_t parse_opt(int key, char* arg, struct argp_state* state);

struct login_arguments {
    struct global_arguments* global;
};

static struct argp_option options[] = {{NULL}};

static char doc[] = "Authenticate with the GitTor server.";

static struct argp argp = {options, parse_opt, "", doc, NULL, NULL, NULL};

static error_t parse_opt(int key,
                         __attribute__((__unused__)) char* arg,
                         __attribute__((__unused__)) struct argp_state* state) {
    switch (key) {
        default:
            return ARGP_ERR_UNKNOWN;
    }

    return 0;
}

extern int gittor_login(struct argp_state* state) {
    // Set defaults arguments for login argumnets
    struct login_arguments args = {0};

    // Prepare arguments array for just login
    int argc = state->argc - state->next + 1;
    char** argv = &state->argv[state->next - 1];
    args.global = state->input;

    // Change the command name to gittor login
    const char name[] = "login";
    size_t argv0len = strlen(state->name) + sizeof(name) + 1;
    char* argv0 = argv[0];
    argv[0] = malloc(argv0len);
    g_snprintf(argv[0], argv0len, "%s %s", state->name, name);

    // Parse arguments
    int err = argp_parse(&argp, argc, argv, ARGP_NO_EXIT, &argc, &args);
    if (err) {
        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return err;
    }

    // Prompt for email/username
    char identifier[256] = {0};
    char password[73] = {0};

    err =
        prompt_line("Email or username", false, identifier, sizeof(identifier));
    if (err) {
        g_printerr("Email or username is required.\n");

        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return err;
    }

    // Determine if it is an email or username
    char email[256] = {0};
    char username[256] = {0};
    if (strchr(identifier, '@')) {
        g_snprintf(email, sizeof(email), "%s", identifier);
    } else {
        g_snprintf(username, sizeof(username), "%s", identifier);
    }

    // Prompt for password
    err = prompt_line("Password", true, password, sizeof(password));
    if (err) {
        g_printerr("Password is required.\n");

        secure_zero(password, sizeof(password));
        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return err;
    }

    // Build the login DTO and clear the password
    login_dto_t login_dto = {.email = g_strdup(email),
                             .username = g_strdup(username),
                             .password = g_strdup(password)};
    secure_zero(password, sizeof(password));

    CURL* curl = api_curl_handle_new();
    if (!curl) {
        login_dto_free(&login_dto);

        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return EIO;
    }

    // Build the URL: /authenticate/login
    char url[512] = {0};
    if (api_build_url(url, sizeof(url), "/authenticate/login")) {
        login_dto_free(&login_dto);
        curl_easy_cleanup(curl);

        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return EINVAL;
    }

    // Serialize the login struct to JSON
    char* json_str = build_login_json(&login_dto);
    if (!json_str) {
        login_dto_free(&login_dto);
        curl_easy_cleanup(curl);

        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return EINVAL;
    }

    // Set up headers and response buffer
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "Accept: application/json");

    response_buf_t response = response_buf_init();

    // Perform request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_POST, 1L);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_str);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    api_result_e check = api_check_response(curl, res);

    login_dto_free(&login_dto);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    g_free(json_str);

    if (check != API_OK) {
        if (check == API_FORBIDDEN) {
            g_printerr("Invalid email/username or password.\n");
        } else if (check == API_SERVER_ERR) {
            g_printerr("Server error occurred. Please try again later.\n");
        } else if (check == API_CURL_ERR) {
            g_printerr("Network error occurred: %s\n", curl_easy_strerror(res));
        } else {
            g_printerr("An unknown error occurred.\n");
        }

        g_free(response.data);

        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return EIO;
    }

    // Parse the returned AuthenticationDto JSON
    authentication_dto_t* auth_dto = parse_authentication_json(response.data);
    g_free(response.data);

    // Persist the authentication data in the global config
    config_id_t key_access_token = {.group = "auth", .key = "access_token"};
    config_id_t key_token_type = {.group = "auth", .key = "token_type"};
    config_id_t key_expires = {.group = "auth", .key = "expires"};

    if (config_set(CONFIG_SCOPE_GLOBAL, &key_access_token,
                   auth_dto->access_token) ||
        config_set(CONFIG_SCOPE_GLOBAL, &key_token_type,
                   auth_dto->token_type) ||
        config_set(CONFIG_SCOPE_GLOBAL, &key_expires, auth_dto->expires)) {
        authentication_dto_free(auth_dto);

        free(argv[0]);
        argv[0] = argv0;
        state->next += argc - 1;

        return EIO;
    }

    // Check if the token is already expired
    time_t now = time(NULL);
    time_t exp = 0;
    if (parse_expiry_epoch(auth_dto->expires, &exp) == 0 && exp <= now) {
        g_printerr(
            "Warning: Received an already expired token. Please log in "
            "again.\n");
        err = EAGAIN;
    } else {
        g_printerr("Login successful.\n");
    }

    authentication_dto_free(auth_dto);

    // Reset back to global
    free(argv[0]);
    argv[0] = argv0;
    state->next += argc - 1;

    return err;
}
