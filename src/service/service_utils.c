#include <service/service_internals.h>

extern int gittor_service_get_port(GError** error) {
    // Evalute the port file path
    gchar* dir = g_build_filename(g_get_user_config_dir(), "gittor", NULL);
    gchar* path = g_build_filename(dir, "service_port", NULL);

    // Attempt to read the contents
    gchar* content = NULL;
    gsize length = 0;
    g_file_get_contents(path, &content, &length, error);

    // Error checking
    if (*error) {
        g_free(content);
        g_free(dir);
        g_free(path);
        return -1;
    }

    if (content == NULL) {
        g_set_error(error, g_quark_from_static_string("get-port"), 1,
                    "Error Reading %s: content null", path);
        g_free(content);
        g_free(dir);
        g_free(path);
        return -1;
    }

    g_strstrip(content);

    if (content[0] == '\0') {
        g_set_error(error, g_quark_from_static_string("get-port"), 2,
                    "Error Reading %s: content empty", path);
        g_free(content);
        g_free(dir);
        g_free(path);
        return -1;
    }

    // Attempt to parse the contents
    gchar* endptr = NULL;
    int port = (int)g_ascii_strtoll(content, &endptr, 10);

    // Error checking
    if (endptr == content || *endptr != '\0') {
        g_set_error(error, g_quark_from_static_string("get-port"), 3,
                    "Error Reading %s: content non-integer", path);
        g_free(content);
        g_free(dir);
        g_free(path);
        return -1;
    }

    // Return the port
    g_free(content);
    g_free(dir);
    g_free(path);
    return port;
}

extern void gittor_servic_set_port(int port, GError** error) {
    // Evalute the port file path
    gchar* dir = g_build_filename(g_get_user_config_dir(), "gittor", NULL);
    if (!g_file_test(dir, G_FILE_TEST_IS_DIR)) {
        if (g_mkdir_with_parents(dir, 0700)) {
            g_set_error(error, g_quark_from_static_string("set-port"), 1,
                        "Error creating configuration directory '%s'", dir);
            return;
        }
    }
    gchar* path = g_build_filename(dir, "service_port", NULL);

    // Generate the port string
    gchar* content = g_strdup_printf("%d\n", port);

    // Write the contents to the file
    g_file_set_contents(path, content, -1, error);

    g_free(dir);
    g_free(path);
    g_free(content);
}

extern int bind_port_in_range(GSocketAddress** addr,
                              const char* ip,
                              GSocket* sock,
                              int start,
                              int end) {
    GError* error = NULL;
    for (int port = start; port <= end; port++) {
        *addr = g_inet_socket_address_new_from_string(ip, port);

        g_socket_bind(sock, *addr, TRUE, &error);
        if (error) {
            g_object_unref(*addr);
            g_clear_error(&error);
            continue;
        }
        return port;
    }
    return -1;
}
