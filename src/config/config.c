#include <glib.h>
#include "config/config.h"

static gchar* local_config_path() {
    gchar* cwd = g_get_current_dir();
    gchar* path = g_build_filename(cwd, ".gittorconfig", NULL);
    g_free(cwd);
    return path;
}

static gchar* global_config_path() {
    const gchar* home = g_get_home_dir();
    gchar* config_path = g_build_filename(home, ".gittorconfig", NULL);
    return config_path;
}

char* config_get(ConfigScope scope,
                 const gchar* group,
                 const gchar* key,
                 const gchar* default_value) {
    GError* error = NULL;
    GKeyFile* keyfile = g_key_file_new();
    gchar* value = NULL;

    // Try local config first
    if (scope == CONFIG_SCOPE_LOCAL) {
        gchar* local_path = local_config_path();
        if (g_file_test(local_path, G_FILE_TEST_EXISTS)) {
            g_key_file_load_from_file(keyfile, local_path, G_KEY_FILE_NONE,
                                      &error);
            if (!error) {
                value = g_key_file_get_string(keyfile, group, key, NULL);
            }
            g_clear_error(&error);
        }
        g_free(local_path);
    }

    // If not found, try global config
    if (!value) {
        gchar* global_path = global_config_path();
        if (g_file_test(global_path, G_FILE_TEST_EXISTS)) {
            g_key_file_load_from_file(keyfile, global_path, G_KEY_FILE_NONE,
                                      &error);
            if (!error) {
                value = g_key_file_get_string(keyfile, group, key, NULL);
            }
            g_clear_error(&error);
        }
        g_free(global_path);
    }

    // If still not found, use default
    if (!value && default_value) {
        value = g_strdup(default_value);
    }

    g_key_file_unref(keyfile);
    return value;
}

int config_set(ConfigScope scope,
               const gchar* group,
               const gchar* key,
               const gchar* value) {
    GError* error = NULL;
    GKeyFile* keyfile = g_key_file_new();

    gchar* path = (scope == CONFIG_SCOPE_LOCAL) ? local_config_path()
                                                : global_config_path();

    GKeyFileFlags flags =
        G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;
    g_key_file_load_from_file(keyfile, path, flags, &error);
    g_clear_error(&error);

    g_key_file_set_string(keyfile, group, key, value);

    gsize length;
    gchar* data = g_key_file_to_data(keyfile, &length, &error);
    if (error) {
        g_printerr("Error exporting config: %s\n", error->message);
        g_clear_error(&error);
        g_key_file_unref(keyfile);
        g_free(path);
        return -1;
    }

    g_file_set_contents(path, data, (gssize)length, &error);
    if (error) {
        g_printerr("Error saving config file: %s\n", error->message);
        g_clear_error(&error);
        g_free(data);
        g_key_file_unref(keyfile);
        g_free(path);
        return -1;
    }

    g_free(data);
    g_free(path);
    g_key_file_unref(keyfile);
    return 0;
}
