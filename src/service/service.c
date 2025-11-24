#include <glib.h>
#include <inttypes.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <gio/gio.h>
#include "service/service.h"
#include "service/service_internals.h"

static void gittor_service_reply(GSocket* client,
                                 const packet_t* msg,
                                 GError** error) {
    header_t header = {.type = msg->type, .len = msg->len, .magic = MAGIC};
    g_socket_send(client, (void*)&header, sizeof(header), NULL, error);
    if (*error) {
        return;
    }

    g_socket_send(client, msg->data, header.len, NULL, error);
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
    void* buffer = NULL;
    packet_t reply;
    char reply_body[256];

    while (run_thread) {
        // Parse the header
        header_t header;
        GError* error = NULL;
        g_socket_receive(client, (void*)&header, sizeof(header), NULL, &error);

        // Error handle
        if (error) {
            g_printerr("[GitTor Service thread=%p] Receive header failed: %s\n",
                       (void*)g_thread_self(), error->message);
            g_clear_error(&error);
            break;
        }

        // Check the header signature
        if (header.magic != MAGIC) {
            g_printerr(
                "[GitTor Service thread=%p] Received incorrect header "
                "signiture: got '%" PRIx64 "' expected '%" PRIx64 "'\n",
                (void*)g_thread_self(), header.magic, MAGIC);
            break;
        }

        // Recieve the body
        free(buffer);
        buffer = NULL;
        if (header.len > 0) {
            buffer = malloc(header.len);
            g_socket_receive(client, buffer, header.len, NULL, &error);
        }

        // Error handle
        if (error) {
            g_printerr("[GitTor Service thread=%p] Receive failed: %s\n",
                       (void*)g_thread_self(), error->message);
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
                printf("[GitTor Service thread=%p] Recieved: '%s'\n",
                       (void*)g_thread_self(), (char*)buffer);
                reply.len = g_snprintf(reply_body, sizeof(reply_body) - 1,
                                       "pid: %d thread: %p", getpid(),
                                       (void*)g_thread_self()) +
                            1;
                reply.type = PING;
                reply.data = reply_body;
                gittor_service_reply(client, &reply, &error);
                if (error) {
                    g_printerr(
                        "[GitTor Service thread=%p] Reply failed: "
                        "%s\n",
                        (void*)g_thread_self(), error->message);
                    run_thread = false;
                }
                break;
            default:
                g_printerr("[GitTor Service thread=%p] Unhandled command: %d\n",
                           (void*)g_thread_self(), header.type);
                reply.len = g_snprintf(reply_body, sizeof(reply_body) - 1,
                                       "Unhandled command: %d", header.type) +
                            1;
                reply.type = ERROR;
                reply.data = reply_body;
                gittor_service_reply(client, &reply, &error);
                if (error) {
                    g_printerr(
                        "[GitTor Service thread=%p] Reply failed: "
                        "%s\n",
                        (void*)g_thread_self(), error->message);
                    run_thread = false;
                }
                break;
        }
    }

    free(buffer);
    g_object_unref(client);
    g_free(data);
    return NULL;
}

extern int gittor_service_main() {
    // Notify that the service is not up
    GError* error = NULL;
    gittor_service_set_port(-1, &error);
    if (error) {
        g_printerr("[GitTor Service] Clear port configuration failed: %s\n",
                   error->message);
        g_clear_error(&error);
        // No need to throw error, things will still work just not as well
    }

    // Create an IPv4 TCP socket
    GSocket* socket = g_socket_new(G_SOCKET_FAMILY_IPV4, G_SOCKET_TYPE_STREAM,
                                   G_SOCKET_PROTOCOL_TCP, &error);
    if (error) {
        g_printerr("[GitTor Service] Error creating socket: %s\n",
                   error->message);
        g_clear_error(&error);
        g_object_unref(socket);
        return 1;
    }

    // Bind to open port on localhost
    GSocketAddress* address = NULL;
    int port = bind_port_in_range(&address, "127.0.0.1", socket, 5000, 6000);
    if (port < 0) {
        g_printerr("[GitTor Service] Bind failed: no valid free ports found\n");
        g_clear_error(&error);
        g_object_unref(socket);
        g_object_unref(address);
        return 1;
    }

    // Start listening
    if (!g_socket_listen(socket, &error)) {
        g_printerr("[GitTor Service] Listen failed: %s\n", error->message);
        g_clear_error(&error);
        g_object_unref(socket);
        g_object_unref(address);
        return 1;
    }

    // Notify that the service is up on this port
    gittor_service_set_port(port, &error);
    if (error) {
        g_printerr("[GitTor Service] Set port configuration failed: %s\n",
                   error->message);
        g_clear_error(&error);
        g_object_unref(socket);
        g_object_unref(address);
        return 1;
    }

    // Establish connections
    GPtrArray* threads = g_ptr_array_new();
    GCancellable* cancellable = g_cancellable_new();
    while (true) {
        // Accept a new connection
        GSocket* client = g_socket_accept(socket, cancellable, &error);

        if (g_cancellable_is_cancelled(cancellable)) {
            g_clear_error(&error);
            break;
        }

        if (error) {
            g_printerr("[GitTor Service] Accept failed: %s\n", error->message);
            g_clear_error(&error);
            continue;
        }

        // Spawn a thread for the new client
        client_thread_data_t* data = g_malloc(sizeof(*data));
        data->client = client;
        data->connection_cancellable = cancellable;
        g_ptr_array_add(threads,
                        g_thread_new("client-handler", handle_client, data));
    }

    // Notify that the service is not up
    gittor_service_set_port(-1, &error);
    if (error) {
        g_printerr("[GitTor Service] Clear port configuration failed: %s\n",
                   error->message);
        g_clear_error(&error);
        // No need to error, things will still work just not as well
    }

    // Cleanup all threads
    for (guint i = 0; i < threads->len; i++) {
        g_thread_join(g_ptr_array_index(threads, i));
    }

    g_ptr_array_free(threads, true);
    g_object_unref(socket);
    g_object_unref(address);
    g_object_unref(cancellable);
    return 0;
}

#ifndef MOCK_GITTOR_SERVICE_RUN
extern int gittor_service_run(bool detached) {
    if (!detached) {
        return gittor_service_main();
    }

    // Notify that the service is not up
    GError* error = NULL;
    gittor_service_set_port(-1, &error);
    if (error) {
        g_printerr("Clear port configuration failed: %s\n", error->message);
        g_clear_error(&error);
        // No need to throw error, things will still work just not as well
    }

    const gchar* argv[] = {g_get_prgname(), "service", "run", NULL};
    g_spawn_async(NULL, (gchar**)argv, NULL, G_SPAWN_SEARCH_PATH, NULL, NULL,
                  NULL, &error);
    if (error) {
        g_printerr("Failed to start service: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    // While port is invalid, wait
    int count = 0;
    while (count < 20) {
        int port = gittor_service_get_port(&error);
        if (!error && port > 0) {
            break;
        }
        if (error) {
            g_clear_error(&error);
        }
        g_usleep(100UL * 1000UL);  // 100 ms
        count++;
    }

    return 0;
}
#endif
