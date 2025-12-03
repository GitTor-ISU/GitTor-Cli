#ifndef CONFIG_CONFIG_H_
#define CONFIG_CONFIG_H_

#include <argp.h>

typedef enum { CONFIG_SCOPE_GLOBAL, CONFIG_SCOPE_LOCAL } ConfigScope;

/**
 * @brief Runs the config subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_config(struct argp_state* state);

/**
 * @brief Gets a configuration value.
 *
 * @param group The configuration group
 * @param key The configuration key
 * @param default_value The default value if the key is not found
 * @return char* The configuration value (must be freed by the caller)
 */
extern char* config_get(ConfigScope scope,
                        const char* group,
                        const char* key,
                        const char* default_value);

/**
 * @brief Sets a configuration value.
 *
 * @param scope The configuration scope (global or local)
 * @param group The configuration group
 * @param key The configuration key
 * @param value The value to set
 * @return int 0 on success, non-zero on failure
 */
extern int config_set(ConfigScope scope,
                      const char* group,
                      const char* key,
                      const char* value);

#endif  // CONFIG_CONFIG_H_
