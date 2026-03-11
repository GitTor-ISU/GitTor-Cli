#include <glib.h>
#include <inttypes.h>
#include <stdint.h>
#include <json-glib/json-glib.h>
#include "api/internal.h"
#include "api/torrents.h"

torrent_dto_t* parse_torrent_json(const char* json_str) {
    JsonParser* parser = json_parser_new();

    // Load JSON data into parser
    if (!json_parser_load_from_data(parser, json_str, -1, NULL)) {
        g_object_unref(parser);
        return NULL;
    }

    // Get the root to make sure it's an object
    JsonNode* root_node = json_parser_get_root(parser);
    if (!root_node || !JSON_NODE_HOLDS_OBJECT(root_node)) {
        g_object_unref(parser);
        return NULL;
    }

    // Parse JSON object into torrent_dto_t
    JsonObject* root_obj = json_node_get_object(root_node);
    torrent_dto_t* dto = g_malloc0(sizeof(torrent_dto_t));

    if (json_object_has_member(root_obj, "id"))
        dto->id = json_object_get_int_member(root_obj, "id");

    if (json_object_has_member(root_obj, "name"))
        dto->name = g_strdup(json_object_get_string_member(root_obj, "name"));

    if (json_object_has_member(root_obj, "description"))
        dto->description =
            g_strdup(json_object_get_string_member(root_obj, "description"));

    if (json_object_has_member(root_obj, "fileSize"))
        dto->file_size = json_object_get_int_member(root_obj, "fileSize");

    if (json_object_has_member(root_obj, "uploaderId"))
        dto->uploader_id =
            (int32_t)json_object_get_int_member(root_obj, "uploaderId");

    if (json_object_has_member(root_obj, "uploaderUsername"))
        dto->uploader_username = g_strdup(
            json_object_get_string_member(root_obj, "uploaderUsername"));

    if (json_object_has_member(root_obj, "createdAt"))
        dto->created_at =
            g_strdup(json_object_get_string_member(root_obj, "createdAt"));

    if (json_object_has_member(root_obj, "updatedAt"))
        dto->updated_at =
            g_strdup(json_object_get_string_member(root_obj, "updatedAt"));

    g_object_unref(parser);
    return dto;
}

extern void torrent_dto_free(torrent_dto_t* dto) {
    if (!dto)
        return;

    g_free(dto->name);
    g_free(dto->description);
    g_free(dto->uploader_username);
    g_free(dto->created_at);
    g_free(dto->updated_at);
    g_free(dto);
}

extern torrent_dto_t* api_get_torrent(int64_t torrent_id,
                                      api_result_e* result) {
    CURL* curl = api_curl_handle_new();
    if (!curl) {
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    // Build the URL
    char url[1024];
    if (api_build_url(url, sizeof(url), "/torrents/%" PRId64, torrent_id)) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_SERVER_ERR;

        return NULL;
    }

    // Set up headers and response buffer
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    headers = api_auth_headers(headers);

    response_buf_t response = response_buf_init();

    // Set curl options and perform request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    api_result_e check = api_check_response(curl, res);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result)
        *result = check;

    if (check != API_OK) {
        g_free(response.data);
        return NULL;
    }

    // Parse JSON response into DTO
    torrent_dto_t* dto = parse_torrent_json(response.data);
    g_free(response.data);

    if (!dto && result)
        *result = API_SERVER_ERR;

    return dto;
}
