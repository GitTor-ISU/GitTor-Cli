#ifndef API_INTERNAL_H_
#define API_INTERNAL_H_

#include <stddef.h>
#include <time.h>
#include <curl/curl.h>

/**
 * @brief Result codes for API operations
 */
typedef enum __attribute__((packed)) {
    API_OK = 0,
    API_BAD_REQUEST,
    API_NOT_FOUND,
    API_FORBIDDEN,
    API_SERVER_ERR,
    API_CURL_ERR,
    API_AUTH_MISSING,
    API_AUTH_INVALID_FORMAT,
    API_AUTH_EXPIRED,
} api_result_e;

/**
 * @brief Buffer structure for storing API response data
 */
typedef struct {
    char* data;
    size_t size;
} response_buf_t;

/**
 * @brief Initialize a response buffer. Caller must free .data when done.
 *
 * @return response_buf_t Ready-to-use response buffer
 */
extern response_buf_t response_buf_init();

/**
 * @brief libcurl write callback for JSON/text responses.
 * Pass to CURLOPT_WRITEFUNCTION; pass a response_buf_t* to CURLOPT_WRITEDATA.
 */
extern size_t write_cb(void* ptr, size_t size, size_t nmemb, void* userdata);

/**
 * @brief libcurl write callback for binary file downloads.
 * Pass to CURLOPT_WRITEFUNCTION; pass a FILE* to CURLOPT_WRITEDATA.
 */
extern size_t write_file_cb(void* ptr, size_t size, size_t nmemb, void* stream);

/**
 * @brief Get the base API URL from config or default. Caller must free the
 * returned string.
 *
 * @return char* The base API URL (e.g. "https://gittor.rent/api/")
 */
extern char* api_get_base_url();

/**
 * @brief Add authentication headers to a curl_slist. Caller should pass a
 * pointer to their existing headers list, and this function will append auth
 * headers if available.
 *
 * @param headers Pointer to the existing curl_slist* of headers (can be NULL)
 * @return api_result_e API_OK if headers were added successfully, or an
 * appropriate error code if authentication info is missing/invalid/expired
 */
extern api_result_e api_auth_headers(struct curl_slist** headers);

/**
 * @brief Create a new CURL handle with standard project settings (timeouts,
 * user-agent, etc).
 *
 * @return CURL* Configured CURL handle, or NULL on failure
 */
extern CURL* api_curl_handle_new();

/**
 * @brief Build a full API URL from a path format string.
 *
 * @param out Output buffer
 * @param out_size Size of the output buffer
 * @param path_fmt Format string for the API path (e.g. "torrents/%ld")
 * @return int 0 on success, non-zero if the URL was truncated
 */
extern int api_build_url(char* out, size_t out_size, const char* path_fmt, ...);

/**
 * @brief Check a completed CURL request for errors.
 *
 * @param curl The CURL handle used for the request
 * @param res The CURLcode result from curl_easy_perform()
 * @return api_result_e appropriate result code
 */
extern api_result_e api_check_response(CURL* curl, CURLcode res);

/**
 * @brief Parse an expiry time string (expected to be in ISO 8601 format) into a
 * time_t epoch value.
 *
 * @param expires The expiry time string to parse
 * @param epoch_out Output parameter for the parsed time_t value
 * @return int 0 on success, non-zero on failure
 */
extern int parse_expiry_epoch(const char* expires, time_t* epoch_out);

#endif  // API_INTERNAL_H_
