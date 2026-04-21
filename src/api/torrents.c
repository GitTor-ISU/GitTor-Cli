#include <glib.h>
#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
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

    if (json_object_has_member(root_obj, "repoId"))
        dto->repo_id =
            g_strdup(json_object_get_string_member(root_obj, "repoId"));

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

char* build_update_json(const torrent_update_t* update) {
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);

    if (update->name) {
        json_builder_set_member_name(builder, "name");
        json_builder_add_string_value(builder, update->name);
    }

    if (update->description) {
        json_builder_set_member_name(builder, "description");
        json_builder_add_string_value(builder, update->description);
    }

    json_builder_end_object(builder);

    JsonGenerator* generator = json_generator_new();
    JsonNode* root = json_builder_get_root(builder);
    json_generator_set_root(generator, root);
    json_node_free(root);

    gsize len = 0;
    char* json_str = json_generator_to_data(generator, &len);

    g_object_unref(generator);
    g_object_unref(builder);

    return json_str;
}

char* build_upload_json(const torrent_upload_t* upload) {
    JsonBuilder* builder = json_builder_new();
    json_builder_begin_object(builder);

    json_builder_set_member_name(builder, "name");
    json_builder_add_string_value(builder, upload->name);

    if (upload->description) {
        json_builder_set_member_name(builder, "description");
        json_builder_add_string_value(builder, upload->description);
    }

    json_builder_set_member_name(builder, "repoId");
    json_builder_add_string_value(builder, upload->repo_id);

    json_builder_end_object(builder);

    JsonGenerator* generator = json_generator_new();
    JsonNode* root = json_builder_get_root(builder);
    json_generator_set_root(generator, root);
    json_node_free(root);

    gsize len = 0;
    char* json_str = json_generator_to_data(generator, &len);

    g_object_unref(generator);
    g_object_unref(builder);

    return json_str;
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

    // Build the URL: /torrents/{id}
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
    api_result_e auth_result = api_auth_headers(&headers);
    if (auth_result != API_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        if (result)
            *result = auth_result;

        return NULL;
    }

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

extern torrent_dto_t* api_get_torrent_by_repo_id(const char* repo_id,
                                                 api_result_e* result) {
    CURL* curl = api_curl_handle_new();
    if (!curl) {
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    // Build the URL: /torrents/repository/{id}
    char url[1024];
    if (api_build_url(url, sizeof(url), "/torrents/repository/%s", repo_id)) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_SERVER_ERR;

        return NULL;
    }

    // Set up headers and response buffer
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    api_result_e auth_result = api_auth_headers(&headers);
    if (auth_result != API_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        if (result)
            *result = auth_result;

        return NULL;
    }

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

extern torrent_dto_t* api_update_torrent(int64_t torrent_id,
                                         const torrent_update_t* update,
                                         api_result_e* result) {
    if (!update) {
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    CURL* curl = api_curl_handle_new();
    if (!curl) {
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    // Build the URL: /torrents/{id}
    char url[1024];
    if (api_build_url(url, sizeof(url), "/torrents/%" PRId64, torrent_id)) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_SERVER_ERR;

        return NULL;
    }

    // Serialize the update struct to JSON
    char* json_str = build_update_json(update);
    if (!json_str) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    // Build multipart form data with the JSON string
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "metadata");
    curl_mime_data(part, json_str, CURL_ZERO_TERMINATED);
    curl_mime_type(part, "application/json");

    // Set up headers and response buffer
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    api_result_e auth_result = api_auth_headers(&headers);
    if (auth_result != API_OK) {
        curl_mime_free(mime);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        if (result)
            *result = auth_result;

        return NULL;
    }

    response_buf_t response = response_buf_init();

    // Configure curl for PUT with multipart body and perform request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    api_result_e check = api_check_response(curl, res);

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    g_free(json_str);

    if (result)
        *result = check;

    if (check != API_OK) {
        g_free(response.data);
        return NULL;
    }

    // Parse the returned TorrentDto JSON
    torrent_dto_t* dto = parse_torrent_json(response.data);
    g_free(response.data);

    if (!dto && result)
        *result = API_SERVER_ERR;

    return dto;
}

extern int api_get_torrent_file(int64_t torrent_id,
                                const char* output_path,
                                api_result_e* result) {
    if (!output_path) {
        if (result)
            *result = API_CURL_ERR;

        return -1;
    }

    CURL* curl = api_curl_handle_new();
    if (!curl) {
        if (result)
            *result = API_CURL_ERR;

        return -1;
    }

    // Build the URL: /torrents/{id}/file
    char url[1024];
    if (api_build_url(url, sizeof(url), "/torrents/%" PRId64 "/file",
                      torrent_id)) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_SERVER_ERR;

        return -1;
    }

    // Open the output file for writing binary
    FILE* fp = fopen(output_path, "wb");
    if (!fp) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_CURL_ERR;

        return -1;
    }

    // Set up headers
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/x-bittorrent");
    api_result_e auth_result = api_auth_headers(&headers);
    if (auth_result != API_OK) {
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        fclose(fp);
        if (result)
            *result = auth_result;

        return -1;
    }

    // Set curl options and perform request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_file_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

    CURLcode res = curl_easy_perform(curl);
    api_result_e check = api_check_response(curl, res);

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    fclose(fp);

    if (result)
        *result = check;

    // Remove partial file on error
    if (check != API_OK) {
        remove(output_path);
        return -1;
    }

    return 0;
}

extern int api_update_torrent_file(int64_t torrent_id,
                                   const char* file_path,
                                   api_result_e* result) {
    if (!file_path) {
        if (result)
            *result = API_CURL_ERR;

        return -1;
    }

    CURL* curl = api_curl_handle_new();
    if (!curl) {
        if (result)
            *result = API_CURL_ERR;

        return -1;
    }

    // Build the URL: /torrents/{id}/file
    char url[1024];
    if (api_build_url(url, sizeof(url), "/torrents/%" PRId64 "/file",
                      torrent_id)) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_SERVER_ERR;

        return -1;
    }

    // Build multipart form data with the .torrent file
    curl_mime* mime = curl_mime_init(curl);
    curl_mimepart* part = curl_mime_addpart(mime);
    curl_mime_name(part, "file");
    curl_mime_filedata(part, file_path);
    curl_mime_type(part, "application/x-bittorrent");

    // Set up headers
    struct curl_slist* headers = NULL;
    api_result_e auth_result = api_auth_headers(&headers);
    if (auth_result != API_OK) {
        curl_mime_free(mime);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        if (result)
            *result = auth_result;

        return -1;
    }

    // Configure curl for PUT with multipart body and perform request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");

    CURLcode res = curl_easy_perform(curl);
    api_result_e check = api_check_response(curl, res);

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);

    if (result)
        *result = check;

    return (check == API_OK) ? 0 : -1;
}

extern torrent_dto_t* api_upload_torrent(const torrent_upload_t* upload,
                                         api_result_e* result) {
    if (!upload || !upload->name || !upload->file_path) {
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    CURL* curl = api_curl_handle_new();
    if (!curl) {
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    // Build the URL: /torrents
    char url[1024];
    if (api_build_url(url, sizeof(url), "/torrents")) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_SERVER_ERR;

        return NULL;
    }

    // Serialize the metadata to JSON
    char* json_str = build_upload_json(upload);
    if (!json_str) {
        curl_easy_cleanup(curl);
        if (result)
            *result = API_CURL_ERR;

        return NULL;
    }

    // Build multipart form data with the JSON string and .torrent file
    curl_mime* mime = curl_mime_init(curl);

    curl_mimepart* meta_part = curl_mime_addpart(mime);
    curl_mime_name(meta_part, "metadata");
    curl_mime_data(meta_part, json_str, CURL_ZERO_TERMINATED);
    curl_mime_type(meta_part, "application/json");

    curl_mimepart* file_part = curl_mime_addpart(mime);
    curl_mime_name(file_part, "file");
    curl_mime_filedata(file_part, upload->file_path);
    curl_mime_type(file_part, "application/x-bittorrent");

    // Set up headers and response buffer
    struct curl_slist* headers = NULL;
    headers = curl_slist_append(headers, "Accept: application/json");
    api_result_e auth_result = api_auth_headers(&headers);
    if (auth_result != API_OK) {
        curl_mime_free(mime);
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        g_free(json_str);
        if (result)
            *result = auth_result;

        return NULL;
    }

    response_buf_t response = response_buf_init();

    // Configure curl for POST with multipart body and perform request
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    api_result_e check = api_check_response(curl, res);

    curl_mime_free(mime);
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    g_free(json_str);

    if (result)
        *result = check;

    if (check != API_OK) {
        g_free(response.data);
        return NULL;
    }

    // Parse the returned TorrentDto JSON
    torrent_dto_t* dto = parse_torrent_json(response.data);
    g_free(response.data);

    if (!dto && result)
        *result = API_SERVER_ERR;

    return dto;
}
