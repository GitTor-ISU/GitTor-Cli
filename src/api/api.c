#include <glib.h>
#include <curl/curl.h>
#include "api/api.h"
#include "config/config.h"

extern int api_init() {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    return (int)res;
}

extern void api_cleanup() {
    curl_global_cleanup();
}

extern int heartbeat() {
    CURL* curl;
    CURLcode res;
    long response_code = 0;  // NOLINT(runtime/int) - required by curl API

    char* endpoint_url =
        config_get(CONFIG_SCOPE_LOCAL,
                   &(config_id_t){.group = "network", .key = "api_url"}, NULL);

    if (!endpoint_url) {
        const char* default_endpoint = "https://gittor.rent/api/";
        endpoint_url = g_strdup(default_endpoint);
        g_print("Network API URL not set in config, using default: %s\n",
                default_endpoint);
    }

    curl = curl_easy_init();
    if (!curl) {
        if (endpoint_url)
            free(endpoint_url);
        return -1;  // Initialization failed
    }

    curl_easy_setopt(curl, CURLOPT_URL, endpoint_url);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);  // 5 seconds timeout

    res = curl_easy_perform(curl);

    if (res == CURLE_OK) {
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        curl_easy_cleanup(curl);

        if (endpoint_url)
            free(endpoint_url);
        return (response_code >= 200 && response_code < 300) ? 0 : -1;
    }

    curl_easy_cleanup(curl);
    if (endpoint_url)
        free(endpoint_url);
    return -1;  // Network/CURL error
}
