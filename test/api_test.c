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

    torrent_dto_t* dto = parse_torrent_json(json);

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
    const char* json =
        "{"
        "  \"id\": 68,"
        "  \"fileSize\": 1,"
        "  \"uploaderId\": 3,"
        "  \"uploaderUsername\": \"janeSmith\","
        "  \"createdAt\": \"2019-08-24T14:15:22Z\","
        "  \"updatedAt\": \"2019-08-24T14:15:22Z\""
        "}";

    torrent_dto_t* dto = parse_torrent_json(json);

    TEST_ASSERT_NOT_NULL(dto);
    TEST_ASSERT_EQUAL_INT64(68, dto->id);
    TEST_ASSERT_NULL(dto->name);
    TEST_ASSERT_NULL(dto->description);

    torrent_dto_free(dto);
}

static void shouldPass_whenParsingInvalidJson(void) {
    TEST_ASSERT_NULL(parse_torrent_json("not valid json {{{"));
}

static void shouldPass_whenFreeingNullDto(void) {
    torrent_dto_free(NULL);
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

    RUN_TEST(shouldPass_whenFreeingNullDto);

    return UNITY_END();
}
