#ifndef INIT_INIT_H_
#define INIT_INIT_H_

#include <argp.h>

#define FILE_URL_MAX (PATH_MAX + 7)

/**
 * @brief Runs the init subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_init(struct argp_state* state);

/**
 * @brief Create a bare repository.
 *
 * @param url Buffer of FILE_URL_MAX size to output the repository URL
 * @return int error code
 */
extern int create_bare_repo(char url[FILE_URL_MAX]);

/**
 * @brief Clone bare repository to local path.
 *
 * @param url URL to the bare repository
 * @param path Local path to the clone location
 * @return int error code
 */
extern int clone_bare_repo(char url[FILE_URL_MAX], char path[PATH_MAX]);

#endif  // INIT_INIT_H_
