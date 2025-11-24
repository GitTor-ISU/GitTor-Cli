#ifndef SERVICE_SERVICE_H_
#define SERVICE_SERVICE_H_

#include <argp.h>
#include <stdbool.h>

typedef enum __attribute__((packed)) {
    /// @brief Kill the service
    KILL,
    /// @brief End the current connection
    END,
    ERROR,
    PING,
    SEED_START,
    SEED_STOP,
    SEED_REMOVE,
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
 * @brief Send a packet to the GitTor service. Starts service if not found.
 *
 * @param msg Message to send
 * @param error Error output
 * @return packet_t* Response
 */
extern packet_t* gittor_service_send(const packet_t* msg, GError** error);

/**
 * @brief Main function for the GitTor service.
 *
 * @return int error code
 */
extern int gittor_service_main();

/**
 * @brief Run the GiTtor service
 *
 * @note This does not check if the service is already running
 * @param detached run the GiTtor service in a detached process
 * @return int error code
 */
extern int gittor_service_run(bool detached);

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
 * @brief Print the current GitTor status
 *
 * @return 'up' or 'down'
 */
extern const char* gittor_service_status();

/**
 * @brief Disconnect from the GiTtor service if connected
 */
extern void gittor_service_disconnect();

/**
 * @brief Send a ping to the GitTor service.
 *
 * @return int error code
 */
extern int gittor_service_ping();

#endif  // SERVICE_SERVICE_H_
