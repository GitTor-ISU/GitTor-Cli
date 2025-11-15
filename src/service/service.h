#ifndef SERVICE_SERVICE_H_
#define SERVICE_SERVICE_H_

#include <argp.h>
#include <stdbool.h>

typedef enum __attribute__((packed)) {
    /// @brief End the current connection
    END,
    /// @brief Kill the service
    KILL,
    PING,
} type_e;

typedef struct {
    void* data;
    ssize_t len;
    type_e type;
} packet_t;

/**
 * @brief Runs the service subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_service(struct argp_state* state);

/**
 * @brief Main function for the GiTtor service
 *
 * @param detached run the GiTtor service in a detached process
 * @return int error code
 */
extern int gittor_service_main(bool detached);

/**
 * @brief Check if the GiTtor service is running and if not, start it
 *
 * @return int error code
 */
extern int gittor_service_start();

/**
 * @brief Check if the GiTtor service is running and if it is, stop it
 *
 * @return int error code
 */
extern int gittor_service_stop();

/**
 * @brief Stop and Start the GiTtor service
 *
 * @return int error code
 */
extern int gittor_service_restart();

/**
 * @brief Print the current gittor status
 *
 * @return int error code
 */
extern int gittor_service_status();

/**
 * @brief Disconnect from the GiTtor service if connected
 */
extern void gittor_service_disconnect();

extern int gittor_service_ping();

#endif  // SERVICE_SERVICE_H_
