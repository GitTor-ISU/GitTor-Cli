#include <glib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <gio/gio.h>
#include "service/service.h"
#include "service/service_internals.h"

static GSocket* s_socket;

/**
 * @brief Attempt to connect to the service
 *
 * @param socket The socket to connect through
 * @param error Error throws
 * @return int error code
 */
static void connect(GSocket* socket, GError** error) {
    // Get the service port
    int port = gittor_service_get_port(error);
    if (error && *error) {
        return;
    } else if (port < 0) {
        g_set_error(error, g_quark_from_static_string(__func__), 1,
                    "Error Connecting: service not found");
        return;
    }

    GSocketAddress* address =
        g_inet_socket_address_new_from_string("127.0.0.1", port);

    // Multiple attempts to connect
    for (int i = 0; i < 5; i++) {
        g_clear_error(error);
        g_socket_connect(socket, address, NULL, error);
        if (!*error) {
            break;
        }
        g_usleep(100UL * 1000UL);  // 100ms
    }

    g_object_unref(address);
    return;
}

/**
 * @brief Get the connection to the service
 *
 * @param auto_start Start the GitTor service if not running
 * @param error Error throws
 * @return GSocket* Connection to the service
 */
static GSocket* get_connection(bool auto_start, GError** error) {
    // Connection has already been established
    if (s_socket != NULL) {
        return s_socket;
    }

    s_socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                            G_SOCKET_PROTOCOL_TCP, error);
    if (error && *error) {
        g_object_unref(s_socket);
        s_socket = NULL;
        return NULL;
    }

    // Attempt to connect
    connect(s_socket, error);

    // If connected, return
    if (!*error) {
        return s_socket;
    } else if (!auto_start) {
        g_object_unref(s_socket);
        s_socket = NULL;
        return NULL;
    }

    g_clear_error(error);

    // Start the gittor service
    gittor_service_run(true);
    g_print("GitTor service started.\n");

    // Attempt to connect a second time
    connect(s_socket, error);

    // If connection fails again, throw error
    if (error && *error) {
        g_object_unref(s_socket);
        s_socket = NULL;
        return NULL;
    }

    return s_socket;
}

extern packet_t* gittor_service_send(const packet_t* msg, GError** error) {
    if (msg->type == END || msg->type == KILL) {
        g_set_error(error, g_quark_from_static_string(__func__), 1,
                    "Error Sending Packet: cannot send control packets");
        return NULL;
    }

    // Get connection to the service
    GSocket* socket = get_connection(true, error);
    if (error && *error) {
        return NULL;
    }

    // Send the header
    header_t header = {.magic = MAGIC, .type = msg->type, .len = msg->len};
    g_socket_send(socket, (void*)&header, sizeof(header), NULL, error);
    if (error && *error) {
        return NULL;
    }

    // Send the data
    g_socket_send(socket, (void*)msg->data, msg->len, NULL, error);
    if (error && *error) {
        return NULL;
    }

    // Recieve the header
    g_socket_receive(socket, (void*)&header, sizeof(header), NULL, error);
    if (error && *error) {
        return NULL;
    }

    if (header.magic != MAGIC) {
        g_set_error(error, g_quark_from_static_string(__func__), 2,
                    "Received Incorrect Header Signiture: got '%" PRIx64
                    "' expected '%" PRIx64 "'\n",
                    header.magic, MAGIC);
        return NULL;
    }

    packet_t* resp = (packet_t*)malloc(sizeof(*resp));
    resp->type = header.type;
    resp->len = header.len;
    resp->data = (void*)malloc(header.len);

    // Recieve the data
    g_socket_receive(socket, resp->data, header.len, NULL, error);
    if (error && *error) {
        free(resp->data);
        free(resp);
        return NULL;
    }

    return resp;
}

extern int gittor_service_start() {
    // Try to connect
    GError* error = NULL;
    get_connection(false, &error);

    if (error) {
        g_clear_error(&error);
        gittor_service_run(true);
        g_print("GitTor service started.\n");
    } else {
        g_print("GitTor service already running.\n");
    }

    return 0;
}

extern int gittor_service_stop() {
    // Try to connect
    GError* error = NULL;
    GSocket* socket = get_connection(false, &error);

    if (!error) {
        // Kill the service
        header_t header = {.magic = MAGIC, .type = KILL, .len = -1};
        g_socket_send(socket, (void*)&header, sizeof(header), NULL, NULL);
        g_object_unref(s_socket);
        s_socket = NULL;
        g_print("GitTor service stopped.\n");
    } else {
        g_clear_error(&error);
        g_print("GitTor service wasn't running.\n");
    }

    if (G_IS_OBJECT(s_socket)) {
        g_object_unref(s_socket);
    }
    s_socket = NULL;

    // While port is greater than 0, wait
    int count = 0;
    while (count < 20) {
        int port = gittor_service_get_port(&error);
        if (error) {
            g_clear_error(&error);
            break;
        }
        if (port <= 0) {
            break;
        }
        g_usleep(100UL * 1000UL);  // 100 ms
        count++;
    }

    return 0;
}

extern int gittor_service_restart() {
    gittor_service_stop();
    gittor_service_start();
    return 0;
}

extern void gittor_service_disconnect() {
    // If already disconnected, skip
    if (s_socket == NULL) {
        return;
    }

    // Tell the service the connection is over
    header_t header = {.magic = MAGIC, .type = END, .len = -1};
    g_socket_send(s_socket, (void*)&header, sizeof(header), NULL, NULL);
    g_object_unref(s_socket);
    s_socket = NULL;
}

extern const char* gittor_service_status() {
    static const char up[] = "up";
    static const char down[] = "down";

    // Try to connect
    GError* error = NULL;
    get_connection(false, &error);

    if (error) {
        g_clear_error(&error);
        return down;
    }

    return up;
}

extern int gittor_service_ping() {
    GError* error = NULL;
    char ping[256];
    packet_t msg = {.data = ping, .type = PING};
    msg.len = g_snprintf(ping, sizeof(ping) - 1, "pid: %d thread: %p", getpid(),
                         (void*)g_thread_self()) +
              1;
    packet_t* resp = gittor_service_send(&msg, &error);

    if (error) {
        free(resp);
        g_printerr("Failed to ping service: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    printf("[GitTor CLI] Recieved: '%s'\n", (char*)resp->data);
    free(resp->data);
    free(resp);
    return 0;
}
