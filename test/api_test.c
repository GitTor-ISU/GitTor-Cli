#include <glib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "api/api.h"
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

int main() {
    UNITY_BEGIN();

    setUp();
    RUN_TEST(shouldPass_whenNetworkAPIUrlIsSet);
    tearDown();

    RUN_TEST(shouldPass_whenNetworkAPIUrlNotSet);

    return UNITY_END();
}
