#ifndef LEECH_LEECH_INTERNAL_H_
#define LEECH_LEECH_INTERNAL_H_

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
 * @return int error code
 */
extern int leech_repository(const char* key, key_type_e type);

#endif  // LEECH_LEECH_INTERNAL_H_
