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
 * @brief Sends a heartbeat signal to the endpoint specified in the config at
 * network.api_url, or defaults to "https://gittor.rent/api/" if not set.
 *
 * @return int 0 on success, non-zero on failure
 */
extern int heartbeat();

#endif  // API_API_H_
