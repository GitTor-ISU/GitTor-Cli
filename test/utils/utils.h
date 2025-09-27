#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize a new temporary directory for testing
 *
 * @param buf The buffer where the path is written
 * @param buflen The length of the buffer
 * @return int error code
 */
extern int tempdir_init(char* buf, size_t buflen);

/**
 * @brief Deletes a temporary directory
 *
 * @param dir The path to the directory
 * @return int error code
 */
extern int tempdir_destroy(char* dir);

/**
 * @brief Checks if the temporary directory exists
 *
 * @param dir The path to the directory
 * @return true if the directory exists
 */
extern bool tempdir_exists(char* dir);

#endif  // UTILS_UTILS_H_
