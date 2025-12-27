#ifndef SEED_SEED_INTERNALS_H_
#define SEED_SEED_INTERNALS_H_

#include <git2.h>
#include <glib.h>

/**
 * @brief Start seeding a GitTor repository given its repository id.
 *
 * @param repo_id Repository ID.
 * @return int error code
 */
extern int gittor_seed_start(const git_oid* repo_id);

/**
 * @brief Stop seeding a GitTor repository given its repository id.
 *
 * @param repo_id Repository ID.
 * @return int error code
 */
extern int gittor_seed_stop(const git_oid* repo_id);

#endif  // SEED_SEED_INTERNALS_H_
