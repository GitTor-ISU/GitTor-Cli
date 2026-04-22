#ifndef SEED_SEED_H_
#define SEED_SEED_H_

#include <argp.h>
#include <glib.h>

/**
 * @brief Runs the seed subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_seed(struct argp_state* state);

/**
 * @brief Start seeding a GitTor repository given its repository id.
 *
 * @param repo_id Repository ID (40-character hex string).
 * @return int error code
 */
extern int gittor_seed_start(const char* repo_id);

/**
 * @brief Stop seeding a GitTor repository given its repository id.
 *
 * @param repo_id Repository ID (40-character hex string).
 * @return int error code
 */
extern int gittor_seed_stop(const char* repo_id);

#endif  // SEED_SEED_H_
