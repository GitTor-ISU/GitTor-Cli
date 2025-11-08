#include <glib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <unistd.h>
#include <gio/gio.h>
#include "service/service.h"
#include "service/service_internals.h"

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

extern int gittor_service_main() {
    // Notify that the service is not up
    GError* error = NULL;
    gittor_servic_set_port(-1, &error);
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
    gittor_servic_set_port(port, &error);
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
    gittor_servic_set_port(-1, &error);
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
