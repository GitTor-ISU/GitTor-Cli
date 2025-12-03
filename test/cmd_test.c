#include <errno.h>
#include <unistd.h>
#include "cmd/cmd.h"
#include "unity/unity.h"
#include "utils/utils.h"

static void shouldPass_whenKnownCommand() {
    // GIVEN: Sub-command call
    char* argv[] = {"gittor", "init", "--help", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldEsrch_whenUnknownCommand() {
    // GIVEN: Unknown sub-command call
    char* argv[] = {"gittor", "unknown", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return no such process error
    TEST_ASSERT_EQUAL(ESRCH, err);
}

static void shouldPass_whenHelpFlag() {
    // GIVEN: Help flag
    char* argv[] = {"gittor", "--help", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenHelpShorthandFlag() {
    // GIVEN: Help flag shorthand
    char* argv[] = {"gittor", "-?", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenUsageFlag() {
    // GIVEN: Help flag
    char* argv[] = {"gittor", "--usage", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenPathWithArg() {
    // GIVEN: Path with argument
    char* argv[] = {"gittor", "--path", "/tmp", "init", "--help", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldEinval_whenPathWithoutArg() {
    // GIVEN: Path without argument
    char* argv[] = {"gittor", "--path", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return invalid argument error
    TEST_ASSERT_EQUAL(EINVAL, err);
}

static void shouldEinval_whenNoArg() {
    // GIVEN: Path without argument
    char* argv[] = {"/home/gittor", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return invalid argument error
    TEST_ASSERT_EQUAL(EINVAL, err);
}

static void shouldEinval_whenUnknownFlag() {
    // GIVEN: Unknown flag
    char* argv[] = {"gittor", "--unknown", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return invalid argument error
    TEST_ASSERT_EQUAL(EINVAL, err);
}

static void shouldEnoent_whenNoCurrentDirectory() {
    char oldwd[PATH_MAX];
    char* tempdir;

    // GIVEN: Any command line call
    char* argv[] = {"gittor", "--help", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // GIVEN: Current directory doesn't exist
    TEST_ASSERT_NOT_NULL(getcwd(oldwd, sizeof(oldwd)));
    tempdir = tempdir_init();
    if (tempdir == NULL) {
        TEST_FAIL_MESSAGE("Failed to create temporary directory");
    }
    TEST_ASSERT_EQUAL(0, chdir(tempdir));
    tempdir_destroy(tempdir);

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return ? error
    TEST_ASSERT_EQUAL(ENOENT, err);

    // Go back to old directory for other tests
    TEST_ASSERT_EQUAL(0, chdir(oldwd));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenKnownCommand);
    RUN_TEST(shouldEsrch_whenUnknownCommand);
    RUN_TEST(shouldPass_whenHelpFlag);
    RUN_TEST(shouldPass_whenHelpShorthandFlag);
    RUN_TEST(shouldPass_whenUsageFlag);
    RUN_TEST(shouldPass_whenPathWithArg);
    RUN_TEST(shouldEinval_whenPathWithoutArg);
    RUN_TEST(shouldEinval_whenNoArg);
    RUN_TEST(shouldEinval_whenUnknownFlag);
    RUN_TEST(shouldEnoent_whenNoCurrentDirectory);
    return UNITY_END();
}
