#ifndef API_API_H_
#define API_API_H_

/**
 * @brief Initialize the API module
 *
 * @return int 0 on success, non-zero on failure
 */
extern int api_init();

/**
 * @brief Cleanup the API module
 */
extern void api_cleanup();

/**
 * @brief Sends a heartbeat signal to the specified endpoint.
 *
 * @param endpoint_url The URL of the endpoint to send the heartbeat to
 * @return int 0 on success, non-zero on failure
 */
extern int heartbeat(const char* endpoint_url);

#endif  // API_API_H_
