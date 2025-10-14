#include <errno.h>
#include "cmd/cmd.h"
#include "unity/unity.h"

static void shouldPass_whenCalledWithNoArgs() {
    // GIVEN: Just calling gittor config
    char* argv[] = {"gittor", "config", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenCalledWithGlobalFlag() {
    // GIVEN: Calling gittor config with --global flag
    char* argv[] = {"gittor", "config", "--global", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenCalledWithLocalFlag() {
    // GIVEN: Calling gittor config with --local flag
    char* argv[] = {"gittor", "config", "--local", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenCalledWithGlobalAndLocalFlags() {
    // GIVEN: Calling gittor config with both --global and --local flags
    char* argv[] = {"gittor", "config", "--global", "--local", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenCalledWithNoArgs);
    RUN_TEST(shouldPass_whenCalledWithGlobalFlag);
    RUN_TEST(shouldPass_whenCalledWithLocalFlag);
    RUN_TEST(shouldPass_whenCalledWithGlobalAndLocalFlags);
    return UNITY_END();
}
