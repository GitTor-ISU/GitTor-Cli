#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "login/login.h"
#include "unity/unity.h"
#include "utils/utils.h"

static char* TEST_DIR = NULL;
static char TEST_GLOBAL_CONFIG_PATH[PATH_MAX + 27];
static char TEST_LOCAL_CONFIG_PATH[PATH_MAX + 27];

static char* original_home = NULL;
static char* original_cwd = NULL;
static int BUFFER_SIZE = 2048;

void setUp() {
    // Save original environment
    original_home = getenv("HOME") ? strdup(getenv("HOME")) : NULL;
    original_cwd = getcwd(NULL, 0);

    // Create test directory
    TEST_DIR = tempdir_init();
    snprintf(TEST_GLOBAL_CONFIG_PATH, sizeof(TEST_GLOBAL_CONFIG_PATH),
             "%s/.test_global_gittorconfig", TEST_DIR);
    snprintf(TEST_LOCAL_CONFIG_PATH, sizeof(TEST_LOCAL_CONFIG_PATH),
             "%s/.test_local_gittorconfig", TEST_DIR);

    // Set HOME to controlled location for global config
    setenv("HOME", TEST_DIR, 1);
    chdir(TEST_DIR);
}

void tearDown() {
    // Remove test config files
    unlink(TEST_GLOBAL_CONFIG_PATH);
    unlink(TEST_LOCAL_CONFIG_PATH);

    // Restore original HOME
    if (original_cwd) {
        chdir(original_cwd);
        free(original_cwd);
        original_cwd = NULL;
    }
    if (original_home) {
        setenv("HOME", original_home, 1);
        free(original_home);
        original_home = NULL;
    }

    // Remove test directory
    tempdir_destroy(TEST_DIR);
}

static void shouldPass_whenParsingWithAllFieldsPresent() {
    // GIVEN: A JSON string with all fields present
    const char* json =
        "{"
        "  \"accessToken\": \"string\","
        "  \"tokenType\": \"string\","
        "  \"expires\": \"2019-08-24T14:15:22Z\""
        "}";

    // WHEN: Parse the JSON into a login_dto_t
    authentication_dto_t* dto = parse_authentication_json(json);

    // THEN: All fields should be parsed correctly
    TEST_ASSERT_NOT_NULL(dto);
    TEST_ASSERT_EQUAL_STRING("string", dto->access_token);
    TEST_ASSERT_EQUAL_STRING("string", dto->token_type);
    TEST_ASSERT_EQUAL_STRING("2019-08-24T14:15:22Z", dto->expires);

    authentication_dto_free(dto);
}

static void shouldPass_whenParsingInvalidJson() {
    // GIVEN: An invalid JSON string
    const char* json = "not valid json {{{";

    // WHEN: Parse the JSON into a torrent_dto_t
    authentication_dto_t* dto = parse_authentication_json(json);

    // THEN: Should return NULL
    TEST_ASSERT_NULL(dto);
}

static void shouldPass_whenLoginWithAllFieldsNull() {
    // GIVEN: A login_dto_t with all fields NULL
    login_dto_t dto = {.email = NULL, .username = NULL, .password = NULL};

    // WHEN: Build JSON string from the update struct
    char* json = build_login_json(&dto);

    // THEN: Should return an empty JSON object
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_EQUAL_STRING("{}", json);

    g_free(json);
}

static void shouldPass_whenLoginWithEmailAndPasswordSet() {
    // GIVEN: A login_dto_t with only email set
    login_dto_t dto = {
        .email = "user@email", .username = NULL, .password = "pass"};

    // WHEN: Build JSON string from the update struct
    char* json = build_login_json(&dto);

    // THEN: Should include only the name field
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"email\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"user@email\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"password\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"pass\""));

    g_free(json);
}

static void shouldPass_whenPromptLineWithoutHiddenInput() {
    // GIVEN: A prompt for input without hidden input
    char buffer[BUFFER_SIZE];
    const char* input = "test input\n";
    FILE* temp_input = tmpfile();
    fputs(input, temp_input);
    rewind(temp_input);
    stdin = temp_input;

    // WHEN: Call prompt_line
    int err = prompt_line("Enter something", false, buffer, sizeof(buffer));

    // THEN: Should read the input correctly
    TEST_ASSERT_EQUAL_MESSAGE(0, err, "prompt_line returned an error");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "test input", buffer, "prompt_line did not read the expected input");

    fclose(temp_input);
}

static void shouldPass_whenPromptLineWithHiddenInput() {
    // GIVEN: A prompt for input with hidden input
    char buffer[BUFFER_SIZE];
    const char* input = "secret\n";
    FILE* temp_input = tmpfile();
    fputs(input, temp_input);
    rewind(temp_input);
    stdin = temp_input;

    // WHEN: Call prompt_line with hide_input = true
    int err = prompt_line("Enter password", true, buffer, sizeof(buffer));

    // THEN: Should read the input correctly
    TEST_ASSERT_EQUAL_MESSAGE(0, err, "prompt_line returned an error");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "secret", buffer, "prompt_line did not read the expected input");

    fclose(temp_input);
}

static void shouldPass_whenSecureZeroWithValidBuffer() {
    // GIVEN: A buffer containing sensitive data
    char buffer[BUFFER_SIZE];
    strncpy(buffer, "sensitive data", sizeof(buffer));

    // WHEN: Call secure_zero on the buffer
    secure_zero(buffer, sizeof(buffer));

    // THEN: The buffer should be zeroed out
    for (size_t i = 0; i < sizeof(buffer); i++) {
        TEST_ASSERT_EQUAL_MESSAGE(
            0, buffer[i], "secure_zero did not zero out the buffer properly");
    }
}

int main() {
    UNITY_BEGIN();

    RUN_TEST(shouldPass_whenParsingWithAllFieldsPresent);

    RUN_TEST(shouldPass_whenParsingInvalidJson);

    RUN_TEST(shouldPass_whenLoginWithAllFieldsNull);

    RUN_TEST(shouldPass_whenLoginWithEmailAndPasswordSet);

    setUp();
    RUN_TEST(shouldPass_whenPromptLineWithoutHiddenInput);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenPromptLineWithHiddenInput);
    tearDown();

    RUN_TEST(shouldPass_whenSecureZeroWithValidBuffer);

    return UNITY_END();
}
