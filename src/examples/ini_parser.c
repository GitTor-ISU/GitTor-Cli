#include <glib.h>
#include "examples/ini_parser.h"

static gchar* config_path() {
    const gchar* home = g_get_home_dir();
    gchar* config_path = g_build_filename(home, ".gittorconfig", NULL);
    return config_path;
}

extern int ini_parse() {
    GError* error = NULL;
    GKeyFile* keyfile = g_key_file_new();
    gchar* path = config_path();

    // Load existing INI file (create if missing)
    GKeyFileFlags flags =
        G_KEY_FILE_KEEP_COMMENTS | G_KEY_FILE_KEEP_TRANSLATIONS;  // NOLINT
    g_key_file_load_from_file(keyfile, path, flags, &error);
    if (error) {
        g_printerr("Error loading file: %s\n", error->message);
        g_clear_error(&error);

        // Default values
        g_key_file_set_string(keyfile, "user", "name", "Isaac Denning");
        g_key_file_set_string(keyfile, "user", "email", "isaac@example.com");
        g_key_file_set_string(keyfile, "user", "tag", "default");
    }

    // Read values
    gchar* name = g_key_file_get_string(keyfile, "user", "name", NULL);
    gchar* email = g_key_file_get_string(keyfile, "user", "email", NULL);
    gchar* tag = g_key_file_get_string(keyfile, "user", "tag", NULL);

    g_print("Name: %s\n", name ? name : "(none)");
    g_print("Email: %s\n", email ? email : "(none)");
    g_print("Tag: %s\n", tag ? tag : "(none)");

    // Modify values
    g_key_file_set_string(keyfile, "user", "tag", "modify");

    // Export data to string
    gsize length;
    gchar* data = g_key_file_to_data(keyfile, &length, &error);
    if (error) {
        g_printerr("Error exporting configuration: %s\n", error->message);
        g_clear_error(&error);
    }

    // Save data to disk
    g_file_set_contents(path, data, (gssize)length, &error);
    if (error) {
        g_printerr("Error saving configuration: %s\n", error->message);
        g_clear_error(&error);
    }

    // Clean up
    g_free(path);
    g_free(data);
    g_free(name);
    g_free(email);
    g_key_file_unref(keyfile);

    return 0;
}
