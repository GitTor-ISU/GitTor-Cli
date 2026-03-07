#include <curl/curl.h>
#include "api/api.h"
#include "api/internal.h"

extern int heartbeat() {
    CURL* curl = api_curl_handle_new();
    if (!curl)
        return -1;

    char url[1024];
    if (api_build_url(url, sizeof(url), "/") != 0) {
        curl_easy_cleanup(curl);
        return -1;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_NOBODY, 5L);  // short timeout for heartbeat

    CURLcode res = curl_easy_perform(curl);
    api_result_e result = api_check_response(curl, res);

    curl_easy_cleanup(curl);
    return (result == API_OK) ? 0 : -1;
}
