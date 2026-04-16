#ifndef LEECH_LEECH_INTERNAL_H_
#define LEECH_LEECH_INTERNAL_H_

#include <stddef.h>

typedef enum __attribute__((packed)) {
    REPO_ID,
    MAGNET_LINK,
    TORRENT_PATH,
    INVALID,
} key_type_e;

/**
 * @brief Leeches an entire repository.
 *
 * @param key The key used to identify the repository to leech
 * @param type The type of key given
 * @param output_path Buffer to store the path of the leeched bare repository
 * (should be at least PATH_MAX size)
 * @param output_path_size Size of the output_path buffer
 * @return int error code
 */
extern int leech_repository(const char* key,
                            key_type_e type,
                            char* output_path,
                            size_t output_path_size);

#endif  // LEECH_LEECH_INTERNAL_H_
