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
    char* repo_id;
    int64_t file_size;
    int32_t uploader_id;
    char* uploader_username;
    char* created_at;
    char* updated_at;
} torrent_dto_t;

/**
 * @brief Input for updating torrent metadata. Only non-NULL fields are sent.
 */
typedef struct {
    const char* name;
    const char* description;
} torrent_update_t;

/**
 * @brief Input for uploading a new torrent. name and file_path are required,
 * description is optional.
 */
typedef struct {
    const char* name;
    const char* description;
    const char* repo_id;
    const char* file_path;
} torrent_upload_t;

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
 * @brief Serialize a torrent_update_t into a JSON string. Internal function for
 * sending API requests.
 *
 * @param update The torrent_update_t to serialize
 * @return char* Allocated JSON string, or NULL on error. Caller must free with
 * g_free().
 */
char* build_update_json(const torrent_update_t* update);

/**
 * @brief Serialize a torrent_upload_t into a JSON string. Internal function for
 * sending API requests.
 *
 * @param upload The torrent_upload_t to serialize
 * @return char* Allocated JSON string, or NULL on error. Caller must free with
 * g_free().
 */
char* build_upload_json(const torrent_upload_t* upload);

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

/**
 * @brief Get torrent metadata by the repository ID. GET
 * /torrents/repository/{id}
 *
 * @param repo_id The first commit of the repository hash
 * @param result Pointer to store the API result code
 * @return torrent_dto_t* The torrent, or NULL on error. Caller must free with
 * torrent_dto_free().
 */
extern torrent_dto_t* api_get_torrent_by_repo_id(const char* repo_id,
                                                 api_result_e* result);
/**
 * @brief Update torrent metadata with non-NULL fields. PUT /torrents/{id}
 *
 * @param torrent_id The torrent ID to update
 * @param update The fields to update (name and/or description, can be NULL)
 * @param result Pointer to store the API result code
 * @return torrent_dto_t* The updated torrent, or NULL on error. Caller must
 * free with torrent_dto_free().
 */
extern torrent_dto_t* api_update_torrent(int64_t torrent_id,
                                         const torrent_update_t* update,
                                         api_result_e* result);

/**
 * @brief Download a .torrent file to disk. GET /torrents/{id}/file
 *
 * @param torrent_id The torrent ID whose file to download
 * @param output_path The file path to save the downloaded .torrent to
 * @param result Pointer to store the API result code
 * @return int 0 on success, non-zero on error
 */
extern int api_get_torrent_file(int64_t torrent_id,
                                const char* output_path,
                                api_result_e* result);

/**
 * @brief Replace the .torrent file for an existing torrent.
 * PUT /torrents/{id}/file
 *
 * @param torrent_id The torrent ID whose file to replace
 * @param file_path Local path to the replacement .torrent file
 * @param result Pointer to store the API result code
 * @return int 0 on success, non-zero on error
 */
extern int api_update_torrent_file(int64_t torrent_id,
                                   const char* file_path,
                                   api_result_e* result);

/**
 * @brief Upload a new torrent with metadata and .torrent file. POST
 * /torrents
 *
 * @param upload The upload parameters (name, file_path, optional
 * description)
 * @param result Pointer to store the API result code
 * @return torrent_dto_t* The created torrent, or NULL on error. Caller must
 * free with torrent_dto_free().
 */
extern torrent_dto_t* api_upload_torrent(const torrent_upload_t* upload,
                                         api_result_e* result);

#endif  // API_TORRENTS_H_
