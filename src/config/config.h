#ifndef CONFIG_CONFIG_H_
#define CONFIG_CONFIG_H_

#include <argp.h>
#include <glib.h>

typedef enum __attribute__((packed)) {
    CONFIG_SCOPE_GLOBAL,
    CONFIG_SCOPE_LOCAL
} config_scope_e;

typedef struct {
    const gchar* group;
    const gchar* key;
} config_id_t;

/**
 * @brief Gets the path to the local config file (.gittorconfig in current
 * directory). Caller must free the returned string.
 *
 * @return gchar* string path to global config file
 */
extern gchar* global_config_path();

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
 * @param scope The configuration scope (global or local)
 * @param id The configuration identifier
 * @param default_value The default value if the key is not found
 * @return char* The configuration value (must be freed by the caller)
 */
extern char* config_get(config_scope_e scope,
                        const config_id_t* id,
                        const gchar* default_value);

/**
 * @brief Sets a configuration value.
 *
 * @param scope The configuration scope (global or local)
 * @param id The configuration identifier
 * @param value The value to set
 * @return int 0 on success, non-zero on failure
 */
extern int config_set(config_scope_e scope,
                      const config_id_t* id,
                      const gchar* value);

#endif  // CONFIG_CONFIG_H_
