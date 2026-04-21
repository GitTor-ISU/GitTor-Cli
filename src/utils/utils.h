#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <git2.h>
#include <limits.h>

/**
 * @brief Get a repositories unique identifier.
 *
 * @param repo_id Output buffer for repository ID
 * @param repo Repository
 * @return int error code
 */
extern int gittor_get_repo_id(git_oid* repo_id, git_repository* repo);

/**
 * @brief Push the repository to the remote.
 */
extern int gittor_git_push(git_repository* repo);

/**
 * @brief Get the directory containing all the repository remotes.
 *
 * @return const char* repository remotes directory
 */
extern const char* gittor_remote_dir();

/**
 * @brief Get the path to the repository's remote.
 *
 * @param buf Output buffer for the path to the repository's remote
 * @param repo_id Repository ID
 * @return int error code
 */
extern int gittor_remote_path(char buf[PATH_MAX], const git_oid* repo_id);

#endif  // UTILS_UTILS_H_
