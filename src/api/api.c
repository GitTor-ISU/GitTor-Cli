#include <curl/curl.h>
#include "api/api.h"

extern int api_init() {
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    return (int)res;
}

extern void api_cleanup() {
    curl_global_cleanup();
}
