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

static void shouldPass_whenUpdateWithAllFieldsNull(void) {
    // GIVEN: A torrent_update_t with both fields NULL
    torrent_update_t update = {.name = NULL, .description = NULL};

    // WHEN: Build JSON string from the update struct
    char* json = build_update_json(&update);

    // THEN: Should return an empty JSON object
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_EQUAL_STRING("{}", json);

    g_free(json);
}

static void shouldPass_whenUpdateWithOnlyNameIsSet(void) {
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

static void shouldPass_whenUpdateWithOnlyDescriptionIsSet(void) {
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

static void shouldPass_whenUpdateWithAllFieldsAreSet(void) {
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

static void shouldPass_whenUploadWithAllFieldsNull(void) {
    // GIVEN: A torrent_upload_t with all fields NULL
    torrent_upload_t upload = {
        .name = NULL, .description = NULL, .file_path = NULL};

    // WHEN: Build JSON string from the upload struct
    char* json = build_upload_json(&upload);

    // THEN: Should return a JSON string
    TEST_ASSERT_NOT_NULL(json);

    g_free(json);
}

static void shouldPass_whenUploadWithOnlyNameIsSet(void) {
    // GIVEN: A torrent_upload_t with only name set
    torrent_upload_t upload = {
        .name = "My Torrent", .description = NULL, .file_path = NULL};

    // WHEN: Build JSON string from the upload struct
    char* json = build_upload_json(&upload);

    // THEN: Should include only the name field
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"name\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"My Torrent\""));
    TEST_ASSERT_NULL(strstr(json, "\"description\""));

    g_free(json);
}

static void shouldPass_whenUploadWithOnlyDescriptionIsSet(void) {
    // GIVEN: A torrent_upload_t with only description set
    torrent_upload_t upload = {
        .name = NULL, .description = "A description", .file_path = NULL};

    // WHEN: Build JSON string from the upload struct
    char* json = build_upload_json(&upload);

    // THEN: Should include only the description field
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"description\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"A description\""));

    g_free(json);
}

static void shouldPass_whenUploadWithRequiredFieldsSet(void) {
    // GIVEN: A torrent_upload_t with required fields set
    torrent_upload_t upload = {.name = "My Torrent",
                               .description = NULL,
                               .file_path = "/tmp/file.torrent"};

    // WHEN: Build JSON string from the upload struct
    char* json = build_upload_json(&upload);

    // THEN: Should include only the name field
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"name\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"My Torrent\""));
    TEST_ASSERT_NULL(strstr(json, "\"description\""));
    TEST_ASSERT_NULL(strstr(json, "\"file_path\""));

    g_free(json);
}

static void shouldPass_whenUploadWithAllFieldsAreSet(void) {
    // GIVEN: A torrent_upload_t with all fields set
    torrent_upload_t upload = {.name = "My Torrent",
                               .description = "A description",
                               .file_path = "/tmp/file.torrent"};

    // WHEN: Build JSON string from the upload struct
    char* json = build_upload_json(&upload);

    // THEN: Should include name and description only
    TEST_ASSERT_NOT_NULL(json);
    TEST_ASSERT_NOT_NULL(strstr(json, "\"name\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"My Torrent\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"description\""));
    TEST_ASSERT_NOT_NULL(strstr(json, "\"A description\""));
    TEST_ASSERT_NULL(strstr(json, "\"file_path\""));

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
    api_init();

    setUp();
    RUN_TEST(shouldPass_whenNetworkAPIUrlIsSet);
    tearDown();

    RUN_TEST(shouldPass_whenNetworkAPIUrlNotSet);

    RUN_TEST(shouldPass_whenParsingWithAllFieldsPresent);

    RUN_TEST(shouldPass_whenParsingWithNullableFields);

    RUN_TEST(shouldPass_whenParsingInvalidJson);

    RUN_TEST(shouldPass_whenUpdateWithAllFieldsNull);

    RUN_TEST(shouldPass_whenUpdateWithOnlyNameIsSet);

    RUN_TEST(shouldPass_whenUpdateWithOnlyDescriptionIsSet);

    RUN_TEST(shouldPass_whenUpdateWithAllFieldsAreSet);

    RUN_TEST(shouldPass_whenUploadWithAllFieldsNull);

    RUN_TEST(shouldPass_whenUploadWithOnlyNameIsSet);

    RUN_TEST(shouldPass_whenUploadWithOnlyDescriptionIsSet);

    RUN_TEST(shouldPass_whenUploadWithRequiredFieldsSet);

    RUN_TEST(shouldPass_whenUploadWithAllFieldsAreSet);

    RUN_TEST(shouldPass_whenFreeingNullDto);

    api_cleanup();
    return UNITY_END();
}
