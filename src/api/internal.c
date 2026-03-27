#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include "api/internal.h"
#include "config/config.h"

static const char DEFAULT_API_URL[] = "https://gittor.rent/api";
static const char USER_AGENT[] = "GitTor-CLI/dev";  // Hardcoded for now

extern response_buf_t response_buf_init() {
    response_buf_t buf;
    buf.data = g_malloc(1);
    buf.data[0] = '\0';
    buf.size = 0;
    return buf;
}

extern size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata) {
    size_t total = size * nmemb;
    response_buf_t* buf = (response_buf_t*)userdata;

    // Reallocate buffer for new data + null terminator
    char* tmp = g_realloc(buf->data, buf->size + total + 1);
    if (!tmp)
        return 0;

    // Append new data
    buf->data = tmp;
    memcpy(buf->data + buf->size, ptr, total);  // NOLINT - safe since realloc
    buf->size += total;
    buf->data[buf->size] = '\0';

    return total;
}

extern size_t write_file_cb(void* ptr,
                            size_t size,
                            size_t nmemb,
                            void* stream) {
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}

extern char* api_get_base_url() {
    // Get the API URL from config
    char* endpoint_url =
        config_get(CONFIG_SCOPE_LOCAL,
                   &(config_id_t){.group = "network", .key = "api_url"}, NULL);

    // If not set, use the default
    if (!endpoint_url) {
        endpoint_url = g_strdup(DEFAULT_API_URL);
        g_print("Network API URL not set in config, using default: %s\n",
                DEFAULT_API_URL);
    }

    // Remove trailing slashes
    size_t len = strlen(endpoint_url);
    while (len > 0 && endpoint_url[len - 1] == '/') {
        endpoint_url[--len] = '\0';
    }

    return endpoint_url;
}

extern char* api_get_token() {
    return config_get(CONFIG_SCOPE_LOCAL,
                      &(config_id_t){.group = "auth", .key = "access_token"},
                      NULL);
}

extern struct curl_slist* api_auth_headers(struct curl_slist* headers) {
    char* token = api_get_token();
    if (token && token[0]) {
        char* hdr = g_strdup_printf("Authorization: Bearer %s", token);
        headers = curl_slist_append(headers, hdr);
        g_free(hdr);
    }

    g_free(token);
    return headers;
}

extern CURL* api_curl_handle_new() {
    CURL* curl = curl_easy_init();
    if (!curl)
        return NULL;

    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);  // 30 second timeout
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, USER_AGENT);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    return curl;
}

extern int api_build_url(char* out,
                         size_t out_size,
                         const char* path_fmt,
                         ...) {
    // Get the base URL
    char* base_url = api_get_base_url();

    int base_len = g_snprintf(out, out_size, "%s", base_url);
    g_free(base_url);

    if (base_len < 0 || (size_t)base_len >= out_size)
        return -1;

    // Append the path
    va_list args;
    va_start(args, path_fmt);
    int path_len =
        g_vsnprintf(out + base_len, out_size - base_len, path_fmt, args);
    va_end(args);

    return (path_len < 0 || (size_t)(base_len + (size_t)path_len) >= out_size)
               ? -1
               : 0;
}

extern api_result_e api_check_response(CURL* curl, CURLcode res) {
    if (res != CURLE_OK)
        return API_CURL_ERR;

    long response_code = 0;  // NOLINT(runtime/int) - required by curl API
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);

    if (response_code >= 200 && response_code < 300)
        return API_OK;

    if (response_code == 400)
        return API_BAD_REQUEST;
    if (response_code == 401 || response_code == 403)
        return API_FORBIDDEN;
    if (response_code == 404)
        return API_NOT_FOUND;

    if (response_code >= 500 && response_code < 600)
        return API_SERVER_ERR;

    printf("Unexpected response code: %ld\n", response_code);
    return API_CURL_ERR;  // Treat other codes as errors
}

extern int parse_expiry_epoch(const char* expires, time_t* epoch_out) {
    if (!expires || expires[0] == '\0' || !epoch_out) {
        return EINVAL;
    }

    errno = 0;
    char* endptr = NULL;
    gint64 value = g_ascii_strtoll(expires, &endptr, 10);
    if (errno != 0 || endptr == expires || *endptr != '\0' || value < 0)
        return EINVAL;

    *epoch_out = (time_t)value;
    return 0;
}
