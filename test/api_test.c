#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "api/api.h"
#include "api/torrents.h"
#include "cmd/cmd.h"
#include "config/config.h"
#include "unity/unity.h"
#include "utils/utils.h"

static char* TEST_DIR = NULL;
static char TEST_GLOBAL_CONFIG_PATH[PATH_MAX + 27];
static char TEST_LOCAL_CONFIG_PATH[PATH_MAX + 27];

static char* original_home = NULL;
static char* original_cwd = NULL;

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

static void shouldPass_whenNetworkAPIUrlIsSet() {
    // GIVEN: A valid endpoint URL
    char* argv[] = {"gittor",
                    "config",
                    "--local",
                    "network.api_url",
                    "https://gittor.rent/api/",
                    NULL};
    cmd_parse(5, argv);

    // WHEN: Call heartbeat
    int err = heartbeat();

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenNetworkAPIUrlNotSet() {
    // WHEN: Call heartbeat
    int err = heartbeat();

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenParsingWithAllFieldsPresent(void) {
    // GIVEN: A JSON string with all fields present
    const char* json =
        "{"
        "  \"id\": 67,"
        "  \"name\": \"awesome-project\","
        "  \"description\": \"An awesome project\","
        "  \"fileSize\": 1,"
        "  \"uploaderId\": 2,"
        "  \"uploaderUsername\": \"johnDoe\","
        "  \"createdAt\": \"2019-08-24T14:15:22Z\","
        "  \"updatedAt\": \"2019-08-24T14:15:22Z\""
        "}";

    // WHEN: Parse the JSON into a torrent_dto_t
    torrent_dto_t* dto = parse_torrent_json(json);

    // THEN: All fields should be parsed correctly
    TEST_ASSERT_NOT_NULL(dto);
    TEST_ASSERT_EQUAL_INT64(67, dto->id);
    TEST_ASSERT_EQUAL_STRING("awesome-project", dto->name);
    TEST_ASSERT_EQUAL_STRING("An awesome project", dto->description);
    TEST_ASSERT_EQUAL_INT64(1, dto->file_size);
    TEST_ASSERT_EQUAL_INT32(2, dto->uploader_id);
    TEST_ASSERT_EQUAL_STRING("johnDoe", dto->uploader_username);
    TEST_ASSERT_EQUAL_STRING("2019-08-24T14:15:22Z", dto->created_at);
    TEST_ASSERT_EQUAL_STRING("2019-08-24T14:15:22Z", dto->updated_at);

    torrent_dto_free(dto);
}

static void shouldPass_whenParsingWithNullableFields(void) {
    // GIVEN: A JSON string with nullable fields set to null
    const char* json =
        "{"
        "  \"id\": 68,"
        "  \"fileSize\": 1,"
        "  \"uploaderId\": 3,"
        "  \"uploaderUsername\": \"janeSmith\","
        "  \"createdAt\": \"2019-08-24T14:15:22Z\","
        "  \"updatedAt\": \"2019-08-24T14:15:22Z\""
        "}";

    // WHEN: Parse the JSON into a torrent_dto_t
    torrent_dto_t* dto = parse_torrent_json(json);

    // THEN: nullable fields should be NULL, others parsed correctly
    TEST_ASSERT_NOT_NULL(dto);
    TEST_ASSERT_EQUAL_INT64(68, dto->id);
    TEST_ASSERT_NULL(dto->name);
    TEST_ASSERT_NULL(dto->description);

    torrent_dto_free(dto);
}

static void shouldPass_whenParsingInvalidJson(void) {
    // GIVEN: An invalid JSON string
    const char* json = "not valid json {{{";

    // WHEN: Parse the JSON into a torrent_dto_t
    torrent_dto_t* dto = parse_torrent_json(json);

    // THEN: Should return NULL
    TEST_ASSERT_NULL(dto);
}
static void shouldPass_whenAllFieldsNull(void) {
    // GIVEN: A torrent_update_t with both fields NULL
    torrent_update_t update = {.name = NULL, .description = NULL};

    // WHEN: Build JSON string from the update struct
    char* json = build_update_json(&update);

    // THEN: Should return an empty JSON object
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_EQUAL_STRING("{}", json);

    g_free(json);
}

static void shouldPass_whenOnlyNameIsSet(void) {
    // GIVEN: A torrent_update_t with only name set
    torrent_update_t update = {.name = "My Torrent", .description = NULL};

    // WHEN: Build JSON string from the update struct
    char* json = build_update_json(&update);

    // THEN: Should include only the name field
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"name\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"My Torrent\""));
    TEST_ASSERT_NULL(strstr(json, "\"description\""));

    g_free(json);
}

static void shouldPass_whenOnlyDescriptionIsSet(void) {
    // GIVEN: A torrent_update_t with only description set
    torrent_update_t update = {.name = NULL, .description = "A description"};

    // WHEN: Build JSON string from the update struct
    char* json = build_update_json(&update);

    // THEN: Should include only the description field
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"description\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"A description\""));
    TEST_ASSERT_NULL(strstr(json, "\"name\""));

    g_free(json);
}

static void shouldPass_whenAllFieldsAreSet(void) {
    // GIVEN: A torrent_update_t with both fields set
    torrent_update_t update = {.name = "My Torrent",
                               .description = "A description"};

    // WHEN: Build JSON string from the update struct
    char* json = build_update_json(&update);

    // THEN: Should include both fields
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"name\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"My Torrent\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"description\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"A description\""));

    g_free(json);
}

static void shouldPass_whenFreeingNullDto(void) {
    // GIVEN: A NULL torrent_dto_t pointer
    torrent_dto_t* dto = NULL;

    // WHEN: Call torrent_dto_free with NULL
    torrent_dto_free(dto);

    // THEN: Shouldn't crash
    TEST_PASS();
}

int main() {
    UNITY_BEGIN();

    setUp();
    RUN_TEST(shouldPass_whenNetworkAPIUrlIsSet);
    tearDown();

    RUN_TEST(shouldPass_whenNetworkAPIUrlNotSet);

    RUN_TEST(shouldPass_whenParsingWithAllFieldsPresent);

    RUN_TEST(shouldPass_whenParsingWithNullableFields);

    RUN_TEST(shouldPass_whenParsingInvalidJson);

    RUN_TEST(shouldPass_whenAllFieldsNull);

    RUN_TEST(shouldPass_whenOnlyNameIsSet);

    RUN_TEST(shouldPass_whenOnlyDescriptionIsSet);

    RUN_TEST(shouldPass_whenAllFieldsAreSet);

    RUN_TEST(shouldPass_whenFreeingNullDto);

    return UNITY_END();
}
