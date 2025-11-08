#ifndef SERVICE_SERVICE_H_
#define SERVICE_SERVICE_H_

#include <argp.h>

/**
 * @brief Runs the service subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_service(struct argp_state* state);

/**
 * @brief Main function for the gittor service
 *
 * @return int error code
 */
extern int gittor_service_main();

/**
 * @brief Check if the gittor service is running and if not, start it
 *
 * @return int error code
 */
extern int gittor_service_start();

/**
 * @brief Check if the gittor service is running and if it is, stop it
 *
 * @return int error code
 */
extern int gittor_service_stop();

/**
 * @brief Stop and Start the gittor service
 *
 * @return int error code
 */
extern int gittor_service_restart();

extern int gittor_service_ping();

#endif  // SERVICE_SERVICE_H_
