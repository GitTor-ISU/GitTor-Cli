#ifndef DEVS_DEVS_H_
#define DEVS_DEVS_H_

#include <argp.h>

/**
 * @brief Runs the devs subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_devs(struct argp_state* state);

#endif  // DEVS_DEVS_H_
