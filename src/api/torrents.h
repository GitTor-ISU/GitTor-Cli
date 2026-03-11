#ifndef API_TORRENTS_H_
#define API_TORRENTS_H_

#include <stdint.h>
#include "api/internal.h"

/**
 * @brief Represents a torrent (repository) returned by the API. Maps to
 * TorrentDto in the backend.
 */
typedef struct {
    int64_t id;
    char* name;
    char* description;
    int64_t file_size;
    int32_t uploader_id;
    char* uploader_username;
    char* created_at;
    char* updated_at;
} torrent_dto_t;

/**
 * @brief Parse a JSON string into a torrent_dto_t. Internal function for
 * parsing API responses.
 *
 * @param json_str The JSON string to parse
 * @return torrent_dto_t* The parsed torrent, or NULL on error. Caller must free
 * with torrent_dto_free().
 */
torrent_dto_t* parse_torrent_json(const char* json_str);

/**
 * @brief Free a torrent_dto_t and its internal string fields.
 *
 * @param dto The torrent_dto_t to free (can be NULL)
 */
extern void torrent_dto_free(torrent_dto_t* dto);

/**
 * @brief Get torrent metadata by ID. GET /torrents/{id}
 *
 * @param torrent_id The torrent ID
 * @param result Pointer to store the API result code
 * @return torrent_dto_t* The torrent, or NULL on error. Caller must free with
 * torrent_dto_free().
 */
extern torrent_dto_t* api_get_torrent(int64_t torrent_id, api_result_e* result);

#endif  // API_TORRENTS_H_
