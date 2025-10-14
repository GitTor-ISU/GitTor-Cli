#ifndef CONFIG_CONFIG_H_
#define CONFIG_CONFIG_H_

#include <argp.h>

/**
 * @brief Runs the config subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_config(struct argp_state* state);

#endif  // CONFIG_CONFIG_H_
