#include <glib.h>
#include <curl/curl.h>
#include "examples/curl.h"

extern int curl_example() {
    CURL* curl;

    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res)
        return (int)res;

    curl = curl_easy_init();
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, "https://example.com");
        /* example.com is redirected, so we tell libcurl to follow redirection
         */
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        /* Perform the request, res gets the return code */
        res = curl_easy_perform(curl);
        /* Check for errors */
        if (res != CURLE_OK)
            g_printerr("curl_easy_perform() failed: %s\n",
                       curl_easy_strerror(res));

        /* always cleanup */
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
    return 0;
}
