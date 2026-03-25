#ifndef LOGIN_LOGIN_H_
#define LOGIN_LOGIN_H_

#include <argp.h>
#include <stdbool.h>

/**
 * @brief Represents the authentication data returned by the API. Maps to
 * AuthenticationDto in the backend.
 */
typedef struct {
    char* access_token;
    char* token_type;
    char* expires;
} authentication_dto_t;

/**
 * @brief Represents the login data returned by the API. Maps to LoginDto in the
 * backend.
 */
typedef struct {
    char* email;
    char* username;
    char* password;
} login_dto_t;

/**
 * @brief Runs the login subcommand.
 *
 * @param state The arguments of the subcommand
 * @return int error code
 */
extern int gittor_login(struct argp_state* state);

/**
 * @brief Parse a JSON string into an authentication_dto_t. Internal function
 * for parsing API responses.
 *
 * @param json The JSON string to parse
 * @return authentication_dto_t* The parsed authentication data, or NULL on
 * error. Caller must free with authentication_dto_free().
 */
extern authentication_dto_t* parse_authentication_json(const char* json);

/**
 * @brief Serialize a login_dto_t into a JSON string. Internal function for
 * sending API requests.
 *
 * @param dto The login data to serialize
 * @return char* Allocated JSON string, or NULL on error. Caller must free
 the returned string with g_free().
 */
extern char* build_login_json(const login_dto_t* dto);

/**
 * @brief Free an authentication_dto_t and its internal string fields.
 *
 * @param dto The authentication_dto_t to free (can be NULL)
 */
extern void authentication_dto_free(authentication_dto_t* dto);

/**
 * @brief Free a login_dto_t and its internal string fields.
 *
 * @param dto The login_dto_t to free (can be NULL)
 */
extern void login_dto_free(login_dto_t* dto);

/**
 * @brief Prompts the user for input, optionally hiding the input (for
 * passwords).
 *
 * @param prompt The prompt to display to the user
 * @param hide_input Whether to hide the user's input (e.g. for passwords)
 * @param buffer The buffer to store the user's input (must be at least
 * buffer_size bytes)
 * @param buffer_size The size of the buffer
 * @return int 0 on success, non-zero on failure
 */
extern int prompt_line(const char* prompt,
                       bool hide_input,
                       char* buffer,
                       size_t buffer_size);

/**
 * @brief Securely zeroes a memory buffer to prevent sensitive data from
 * lingering in memory.
 *
 * @param ptr The pointer to the buffer to zero (out must be non-NULL)
 * @param len The length of the buffer in bytes
 */
extern void secure_zero(void* ptr, size_t len);

#endif  // LOGIN_LOGIN_H_
