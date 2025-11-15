#ifndef SERVICE_SERVICE_INTERNALS_H_
#define SERVICE_SERVICE_INTERNALS_H_

#include <glib.h>
#include <gio/gio.h>
#include <service/service.h>

/// @brief Magic value to sign the top of every header
static const guint64 MAGIC = ((guint64)'g' << 40) | ((guint64)'i' << 32) |
                             ((guint64)'t' << 24) | ((guint64)'t' << 16) |
                             ((guint64)'o' << 8) | ((guint64)'r' << 0);

/**
 * @brief Packet that comes before the body
 * @note Body is not always necessary/expected depending on the header
 * contents
 */
typedef struct __attribute__((packed)) {
    guint64 magic;
    type_e type;
    gint64 len;
} header_t;

/// @brief Data passed to each client connection thread
typedef struct {
    GSocket* client;
    /// @brief Call from thread to kill the service
    GCancellable* connection_cancellable;
} client_thread_data_t;

/**
 * @brief Get the currently used port
 *
 * @return int
 *      >0 port
 *      <0 no port found
 */
extern int gittor_service_get_port(GError** error);

/**
 * @brief Set the currently used port
 */
extern void gittor_service_set_port(int port, GError** error);

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
extern int bind_port_in_range(GSocketAddress** addr,
                              const char* ip,
                              GSocket* sock,
                              int start,
                              int end);
#endif  // SERVICE_SERVICE_INTERNALS_H_
