#ifndef UTILS_UTILS_H_
#define UTILS_UTILS_H_

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

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

/**
 * @brief Redirects stdout to a temporary file
 *
 * @param old_stdout_fd Pointer to store the old stdout file descriptor
 * @return FILE* The temporary file
 */
extern FILE* redirect_stdout_to_temp(int* old_stdout_fd);

/**
 * @brief Restores stdout from the old file descriptor
 *
 * @param old_stdout_fd The old stdout file descriptor
 */
extern void restore_stdout(int old_stdout_fd);

/**
 * @brief Reads the contents of the temporary file into a buffer
 *
 * @param temp The temporary file
 * @param buffer The buffer to read into
 * @param size The size of the buffer
 */
extern void read_temp_file(FILE* temp, char* buffer, size_t size);

#endif  // UTILS_UTILS_H_
