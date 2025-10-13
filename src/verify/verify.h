#ifndef VERIFY_VERIFY_H_
#define VERIFY_VERIFY_H_

#include <argp.h>

/**
 * @brief Runs the verify subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_verify(struct argp_state* state);

#endif  // VERIFY_VERIFY_H_
