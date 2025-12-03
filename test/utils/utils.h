#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <stdbool.h>
#include <stddef.h>

/**
 * @brief Initialize a new temporary directory for testing
 *
 * @return char* Temporary directory
 */
extern char* tempdir_init();

/**
 * @brief Deletes a temporary directory
 *
 * @param dir The path to the directory
 */
extern void tempdir_destroy(char* dir);

/**
 * @brief Checks if the temporary directory exists
 *
 * @param dir The path to the directory
 * @return true if the directory exists
 */
extern bool tempdir_exists(char* dir);

#endif  // UTILS_UTILS_H_
