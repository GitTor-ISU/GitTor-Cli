#ifndef SEED_SEED_H_
#define SEED_SEED_H_

#include <argp.h>

/**
 * @brief Runs the seed subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_seed(struct argp_state* state);

#endif  // SEED_SEED_H_
