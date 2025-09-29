#ifndef INIT_INIT_H_
#define INIT_INIT_H_

#include <argp.h>

/**
 * @brief Runs the init subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_init(struct argp_state* state);

#endif  // INIT_INIT_H_
