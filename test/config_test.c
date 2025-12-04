#include <argp.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include "cmd/cmd.h"
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

static void shouldPass_whenSetWithTooManyArguments() {
    // GIVEN: Too many arguments
    char* argv[] = {"gittor", "config", "user.name", "Alice", "ExtraArg", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return EINVAL error
    TEST_ASSERT_NOT_EQUAL(0, err);
}

static void shouldPass_whenSetWithMissingKey() {
    // GIVEN: Missing key argument
    char* argv[] = {"gittor", "config", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return EINVAL error
    TEST_ASSERT_EQUAL(EINVAL, err);
}

static void shouldPass_whenSetWithInvalidKeyFormat() {
    // GIVEN: Invalid key format (missing dot)
    char* argv[] = {"gittor", "config", "username", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return EINVAL error
    TEST_ASSERT_EQUAL(EINVAL, err);
}

static void shouldPass_whenGetWithoutScopeFlag() {
    // GIVEN: Configuration value set
    char* set_argv[] = {"gittor",    "config",       "--global",
                        "user.name", "Global Alice", NULL};
    cmd_parse(5, set_argv);

    char* set_argv2[] = {"gittor",    "config",      "--local",
                         "user.name", "Local Alice", NULL};
    cmd_parse(5, set_argv2);

    // WHEN: Get the value
    int old_stdout;
    FILE* temp = redirect_stdout_to_temp(&old_stdout);

    char* get_argv[] = {"gittor", "config", "user.name", NULL};
    int get_err = cmd_parse(3, get_argv);

    restore_stdout(old_stdout);

    char output[BUFFER_SIZE];
    read_temp_file(temp, output, BUFFER_SIZE);

    // THEN: Should succeed and retrieve local value
    TEST_ASSERT_EQUAL_MESSAGE(0, get_err, "Failed to get local config");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "Local Alice", output, "Incorrect value retrieved from local config");
}

static void shouldPass_whenGetWithLocalFlag() {
    // GIVEN: Configuration value set using local scope
    char* set_argv[] = {"gittor", "config", "--local", "user.email",
                        "alice@example.com"};
    int set_err = cmd_parse(5, set_argv);
    TEST_ASSERT_EQUAL_MESSAGE(0, set_err, "Failed to set local config");

    // WHEN: Get the value
    int old_stdout;
    FILE* temp = redirect_stdout_to_temp(&old_stdout);
    if (!temp) {
        TEST_FAIL_MESSAGE("Failed to redirect stdout to temp file");
    }

    char* get_argv[] = {"gittor", "config", "--local", "user.email", NULL};
    int get_err = cmd_parse(4, get_argv);

    restore_stdout(old_stdout);

    char output[BUFFER_SIZE];
    read_temp_file(temp, output, BUFFER_SIZE);

    // THEN: Should succeed
    TEST_ASSERT_EQUAL_MESSAGE(0, get_err, "Failed to get local config");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "alice@example.com", output,
        "Incorrect value retrieved from local config");
}

static void shouldPass_whenGetWithGlobalFlag() {
    // GIVEN: Configuration value set using global scope
    char* set_argv[] = {"gittor",      "config", "--global",
                        "core.editor", "vim",    NULL};
    cmd_parse(5, set_argv);

    // WHEN: Get the value
    int old_stdout;
    FILE* temp = redirect_stdout_to_temp(&old_stdout);

    char* get_argv[] = {"gittor", "config", "--global", "core.editor", NULL};
    int get_err = cmd_parse(4, get_argv);

    restore_stdout(old_stdout);

    char output[BUFFER_SIZE];
    read_temp_file(temp, output, BUFFER_SIZE);

    // THEN: Should succeed
    TEST_ASSERT_EQUAL_MESSAGE(0, get_err, "Failed to get global config");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "vim", output, "Incorrect value retrieved from global config");
}

static void shouldPass_whenGetWithLocalAndGlobalFlag() {
    // GIVEN: Configuration value set in local scope
    char* global_argv[] = {"gittor",      "config", "--local",
                           "core.editor", "vim",    NULL};
    cmd_parse(5, global_argv);

    // WHEN: Get the value when specifying both scopes (should get local)
    int old_stdout;
    FILE* temp = redirect_stdout_to_temp(&old_stdout);

    char* get_argv[] = {"gittor",   "config",      "--local",
                        "--global", "core.editor", NULL};
    int get_err = cmd_parse(5, get_argv);

    restore_stdout(old_stdout);

    char output[BUFFER_SIZE];
    read_temp_file(temp, output, BUFFER_SIZE);

    // THEN: Should retrieve local value
    TEST_ASSERT_EQUAL_MESSAGE(0, get_err, "Failed to get config");
    TEST_ASSERT_EQUAL_STRING_MESSAGE(
        "vim", output, "Should retrieve local config value over global");
}

static void shouldPass_whenGetMustFallback() {
    // GIVEN: Configuration value set only in global scope
    char* global_argv[] = {"gittor",        "config", "--global",
                           "core.autocrlf", "false",  NULL};
    cmd_parse(5, global_argv);

    // WHEN: Get the value without specifying scope (should fallback to global)
    int old_stdout;
    FILE* temp = redirect_stdout_to_temp(&old_stdout);

    char* get_argv[] = {"gittor", "config", "core.autocrlf", NULL};
    int get_err = cmd_parse(3, get_argv);

    restore_stdout(old_stdout);

    char output[BUFFER_SIZE];
    read_temp_file(temp, output, BUFFER_SIZE);

    // THEN: Should retrieve global value
    TEST_ASSERT_EQUAL_MESSAGE(0, get_err, "Failed to get config");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("false", output,
                                     "Should retrieve global config value");
}

static void shouldPass_whenGetCannotFindGlobal() {
    // GIVEN: Configuration value set only in local scope
    char* local_argv[] = {"gittor",          "config", "--local",
                          "core.whitespace", "fix",    NULL};
    cmd_parse(5, local_argv);

    // WHEN: Try to read from global
    int old_stdout;
    FILE* temp = redirect_stdout_to_temp(&old_stdout);

    char* get_argv[] = {"gittor", "config", "--global", "core.whitespace",
                        NULL};
    int get_err = cmd_parse(4, get_argv);

    restore_stdout(old_stdout);

    char output[BUFFER_SIZE];
    read_temp_file(temp, output, BUFFER_SIZE);

    // THEN: Should not find global
    TEST_ASSERT_EQUAL_MESSAGE(ENOENT, get_err,
                              "Did not return error getting config");
}

int main() {
    UNITY_BEGIN();

    setUp();
    RUN_TEST(shouldPass_whenSetWithTooManyArguments);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenSetWithMissingKey);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenSetWithInvalidKeyFormat);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenGetWithoutScopeFlag);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenGetWithLocalFlag);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenGetWithGlobalFlag);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenGetWithLocalAndGlobalFlag);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenGetMustFallback);
    tearDown();

    setUp();
    RUN_TEST(shouldPass_whenGetCannotFindGlobal);
    tearDown();

    return UNITY_END();
}
