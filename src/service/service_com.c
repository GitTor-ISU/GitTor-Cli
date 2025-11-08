#include <glib.h>
#include <inttypes.h>
#include <stdio.h>
#include <unistd.h>
#include <gio/gio.h>
#include "service/service.h"
#include "service/service_internals.h"

/**
 * @brief Attempt to connect to the service
 *
 * @return int error code
 */
static void connect(GSocket* socket, GError** error) {
    // Get the service port
    int port = gittor_service_get_port(error);
    if (*error) {
        return;
    } else if (port < 0) {
        g_set_error(error, g_quark_from_static_string("connect"), 1,
                    "Error Connecting: service not found");
        return;
    }

    GSocketAddress* address =
        g_inet_socket_address_new_from_string("127.0.0.1", port);

    // Multiple attempts to connect
    for (int i = 0; i < 10; i++) {
        g_clear_error(error);
        if (g_socket_connect(socket, address, NULL, error)) {
            break;
        }
        g_usleep(200UL * 1000UL);
    }

    g_object_unref(address);
    return;
}

extern int gittor_service_start() {
    // Create socket
    GError* error = NULL;
    GSocket* socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                                   G_SOCKET_PROTOCOL_TCP, &error);
    if (!socket) {
        g_printerr("Error Creating Socket: %s\n", error->message);
        return 1;
    }

    // Try to connect
    connect(socket, &error);
    if (!error) {
        g_printerr("GitTor service already running.\n");

        // End the connection
        header_t header = {.magic = MAGIC, .type = END, .length = -1};
        g_socket_send(socket, (void*)&header, sizeof(header), NULL, NULL);
    } else {
        g_clear_error(&error);

        // Start gittor service
        const gchar* argv[] = {g_get_prgname(), "service", "run", NULL};
        if (!g_spawn_async(NULL, (gchar**)argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
                           NULL, NULL, &error)) {
            g_printerr("Failed To Start Service: %s\n", error->message);
            g_clear_error(&error);
            return 2;
        }

        g_print("GitTor service started...\n");
    }

    g_clear_error(&error);
    g_object_unref(socket);
    return 0;
}

extern int gittor_service_stop() {
    // Create socket
    GError* error = NULL;
    GSocket* socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                                   G_SOCKET_PROTOCOL_TCP, &error);
    if (!socket) {
        g_printerr("Error Creating Socket: %s\n", error->message);
        return 1;
    }

    // Try to connect
    connect(socket, &error);
    if (!error) {
        // Kill the service
        header_t header = {.magic = MAGIC, .type = KILL, .length = -1};
        g_socket_send(socket, (void*)&header, sizeof(header), NULL, NULL);
        g_print("GitTor service stopped.\n");
    } else {
        g_clear_error(&error);
        g_printerr("GitTor service wasn't running.\n");
    }

    g_object_unref(socket);
    return 0;
}

extern int gittor_service_restart() {
    // Create socket
    GError* error = NULL;
    GSocket* socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                                   G_SOCKET_PROTOCOL_TCP, &error);
    if (!socket) {
        g_printerr("Error Creating Socket: %s\n", error->message);
        return 1;
    }

    // Kill the service if it exists
    connect(socket, &error);
    if (!error) {
        header_t header = {.magic = MAGIC, .type = KILL, .length = -1};
        g_socket_send(socket, (void*)&header, sizeof(header), NULL, NULL);
        g_print("GitTor service stopped.\n");
    } else {
        g_clear_error(&error);
    }
    g_object_unref(socket);

    // Start gittor service
    const gchar* argv[] = {g_get_prgname(), "service", "run", NULL};
    if (!g_spawn_async(NULL, (gchar**)argv, NULL, G_SPAWN_SEARCH_PATH, NULL,
                       NULL, NULL, &error)) {
        g_printerr("Failed To Start Service: %s\n", error->message);
        g_clear_error(&error);
        return 2;
    }
    return 0;
}

extern int gittor_service_ping() {
    // Create socket
    GError* error = NULL;
    GSocket* socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                                   G_SOCKET_PROTOCOL_TCP, &error);
    if (!socket) {
        g_printerr("Error Creating Socket: %s\n", error->message);
        return 1;
    }

    // Connect to the service
    connect(socket, &error);
    if (error) {
        g_printerr("Error Pinging GitTor Service: %s\n", error->message);
        g_clear_error(&error);
        g_object_unref(socket);
        return 2;
    }

    // Send the ping
    char ping[] = "Hello Server!";
    header_t header = {.magic = MAGIC, .type = PING, .length = sizeof(ping)};
    g_socket_send(socket, (void*)&header, sizeof(header), NULL, NULL);
    g_socket_send(socket, (void*)ping, sizeof(ping), NULL, NULL);
    g_socket_receive(socket, (void*)&header, sizeof(header), NULL, NULL);
    void* buffer = malloc(header.length);
    g_socket_receive(socket, buffer, header.length, NULL, NULL);
    g_print("%s\n", (char*)buffer);
    header.type = END;
    header.length = -1;
    g_socket_send(socket, (void*)&header, sizeof(header), NULL, NULL);

    g_object_unref(socket);
    return 0;
}
