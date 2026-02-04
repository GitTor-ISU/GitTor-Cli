#include <curl/curl.h>
#include "api/api.h"

extern int api_init() {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    return (int)res;
}

extern void api_cleanup() {
    curl_global_cleanup();
}

extern int heartbeat(const char* endpoint_url) {
    CURL* curl;
    CURLcode res;
    long response_code = 0;

    if (!endpoint_url) {
        return -1;  // Invalid URL
    }

    curl = curl_easy_init();
    if (!curl)
        return -1;  // Initialization failed

    curl_easy_setopt(curl, CURLOPT_URL, endpoint_url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);  // 5 seconds timeout

    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_cleanup(curl);

        return (response_code >= 200 && response_code < 300) ? 0 : -1;
    }

    curl_easy_cleanup(curl);
    return -1;  // Network/CURL error
}
