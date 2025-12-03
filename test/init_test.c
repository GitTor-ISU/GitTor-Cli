#include <errno.h>
#include <glib.h>
#include <unistd.h>
#include "cmd/cmd.h"
#include "unity/unity.h"
#include "utils/utils.h"

static void shouldPass_whenHelpFlag() {
    // GIVEN: Init with help flag
    char* argv[] = {"gittor", "init", "--help", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenUsageFlag() {
    // GIVEN: Init with help flag
    char* argv[] = {"gittor", "init", "--usage", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenDirectoryProvided() {
    // Create temporary directory for repo
    gchar* dir = tempdir_init();
    if (dir == NULL) {
        TEST_FAIL_MESSAGE("Failed to create temporary directory");
    }

    // GIVEN: Init with repository name as directory
    char* argv[] = {"gittor", "-p", dir, "init", "repoName", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);
    tempdir_destroy(dir);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenDirectoryEmpty() {
    // Create temporary directory for repo
    gchar* dir = tempdir_init();
    if (dir == NULL) {
        TEST_FAIL_MESSAGE("Failed to create temporary directory");
    }

    // GIVEN: Init without repository name
    char* argv[] = {"gittor", "-p", dir, "init", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);
    tempdir_destroy(dir);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenHelpFlag);
    RUN_TEST(shouldPass_whenUsageFlag);
    RUN_TEST(shouldPass_whenDirectoryProvided);
    RUN_TEST(shouldPass_whenDirectoryEmpty);
    return UNITY_END();
}
