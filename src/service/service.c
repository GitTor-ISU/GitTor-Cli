#include <glib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <gio/gio.h>
#include "service/service.h"

typedef enum __attribute__((packed)) {
    PING,
    /// @brief End the current connection
    END,
    /// @brief Kill the service
    KILL,
} type_e;

/**
 * @brief Packet that comes before the body
 * @note Body is not always necessary/expected depending on the header contents
 */
typedef struct __attribute__((packed)) {
    guint64 magic;
    type_e type;
    guint32 length;
} header_t;

/// @brief Data passed to each client connection thread
typedef struct {
    GSocket* client;
    /// @brief Call from thread to kill the service
    GCancellable* connection_cancellable;
} client_thread_data_t;

/// @brief Magic value to sign the top of every header
static const guint64 MAGIC = ((guint64)'g' << 40) | ((guint64)'i' << 32) |
                             ((guint64)'t' << 24) | ((guint64)'t' << 16) |
                             ((guint64)'o' << 8) | ((guint64)'r' << 0);

/**
 * @brief Set the currently used port
 */
static void set_port(int port, GError** error) {
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

/**
 * @brief Get the currently used port
 *
 * @return int
 *      >0 port
 *      <0 no port found
 */
static int get_port(GError** error) {
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

/**
 * @brief Bind the socket to a port in the range
 *
 * @param addr The address pointer to write to
 * @param ip The address to bind to i.e. 127.0.0.1
 * @param sock The socket to bind to
 * @param start The start of the port range
 * @param end The end of the port range
 * @return int
 *      >0 port
 *      <0 no free port found
 */
int bind_port_in_range(GSocketAddress** addr,
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

/**
 * @brief Attempt to connect to the service
 *
 * @return int error code
 */
static void connect(GSocket* socket, GError** error) {
    // Get the service port
    int port = get_port(error);
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

    // If none succeed, error out
    if (*error) {
        g_object_unref(address);
        return;
    }
    g_object_unref(address);
    return;
}

/**
 * @brief Thread function to handle a client connection to the service
 *
 * @param data The client context
 * @return gpointer NULL
 */
static gpointer handle_client(gpointer data) {
    GSocket* client = ((client_thread_data_t*)data)->client;
    bool run_thread = true;

    while (run_thread) {
        // Parse the header
        header_t header;
        GError* error = NULL;
        g_socket_receive(client, (void*)&header, sizeof(header), NULL, &error);

        // Error handle
        if (error) {
            g_printerr("[Thread %p] Receive header failed: %s\n",
                       (void*)g_thread_self(), error->message);
            g_clear_error(&error);
            break;
        }

        // Check the header signature
        if (header.magic != MAGIC) {
            g_printerr(
                "[Thread %p] Received header signiture incorrect: got '%" PRIx64
                "' expected '%" PRIx64 "'\n",
                (void*)g_thread_self(), header.magic, MAGIC);
            break;
        }

        // Recieve the body
        void* buffer = NULL;
        if (header.length > 0) {
            buffer = malloc(header.length);
            g_socket_receive(client, buffer, header.length, NULL, &error);
        }

        // Error handle
        if (error) {
            g_printerr("Receive failed: %s\n", error->message);
            g_clear_error(&error);
            break;
        }

        // Handle the message
        switch (header.type) {
            case KILL:
                run_thread = false;
                g_cancellable_cancel(
                    ((client_thread_data_t*)data)->connection_cancellable);
                break;
            case END:
                run_thread = false;
                break;
            case PING:
                g_print("[Thread %p] Received PING '%s', replying...\n",
                        (void*)g_thread_self(), (char*)buffer);
                char ping[256];
                g_snprintf(ping, sizeof(ping) - 1,
                           "Reply from pid: %d thread: %p", getpid(),
                           (void*)g_thread_self());
                header.type = PING;
                header.length = sizeof(ping);
                g_socket_send(client, (void*)&header, sizeof(header), NULL,
                              NULL);
                g_socket_send(client, (void*)ping, sizeof(ping), NULL, NULL);
                break;
        }
    }

    g_object_unref(client);
    g_free(data);
    return NULL;
}

extern int gittor_service_run() {
    // Notify that the service is not up
    GError* error = NULL;
    set_port(-1, &error);
    if (error) {
        g_printerr("Clear Port Configuration Failed: %s\n", error->message);
        g_clear_error(&error);
        // No need to error, things will still work just not as well
    }

    // Create an IPv4 TCP socket
    GSocket* socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                                   G_SOCKET_PROTOCOL_TCP, &error);
    if (error) {
        g_printerr("Error Creating Socket: %s\n", error->message);
        g_clear_error(&error);
        g_object_unref(socket);
        return 1;
    }

    // Bind to open port on localhost
    GSocketAddress* address = NULL;
    int port = bind_port_in_range(&address, "127.0.0.1", socket, 5000, 6000);
    if (port < 0) {
        g_printerr("Bind Failed: no valid free ports found\n");
        g_clear_error(&error);
        g_object_unref(socket);
        g_object_unref(address);
        return 1;
    }

    // Start listening
    if (!g_socket_listen(socket, &error)) {
        g_printerr("Listen Failed: %s\n", error->message);
        g_clear_error(&error);
        g_object_unref(socket);
        g_object_unref(address);
        return 1;
    }

    // Notify that the service is up on this port
    set_port(port, &error);
    if (error) {
        g_printerr("Set Port Configuration Failed: %s\n", error->message);
        g_clear_error(&error);
        g_object_unref(socket);
        g_object_unref(address);
        return 1;
    }

    // Establish connections
    GCancellable* cancellable = g_cancellable_new();
    while (true) {
        // Accept a new connection
        GSocket* client = g_socket_accept(socket, cancellable, &error);

        if (g_cancellable_is_cancelled(cancellable)) {
            g_clear_error(&error);
            break;
        }

        if (error) {
            g_printerr("Accept failed: %s\n", error->message);
            g_clear_error(&error);
            continue;
        }

        // Spawn a thread for the new client
        client_thread_data_t* data = g_malloc(sizeof(*data));
        data->client = client;
        data->connection_cancellable = cancellable;
        g_thread_new("client-handler", handle_client, data);
    }

    // Notify that the service is not up
    set_port(-1, &error);
    if (error) {
        g_printerr("Clear Port Configuration Failed: %s\n", error->message);
        g_clear_error(&error);
        // No need to error, things will still work just not as well
    }

    g_object_unref(socket);
    g_object_unref(address);
    g_object_unref(cancellable);
    return 0;
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
