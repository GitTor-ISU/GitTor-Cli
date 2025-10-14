#include <errno.h>
#include "cmd/cmd.h"
#include "unity/unity.h"

static void shouldPass_whenCalledWithNoArgs() {
    // GIVEN: Just calling gittor verify
    char* argv[] = {"gittor", "verify", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenCalledWithBranchFlag() {
    // GIVEN: Calling gittor verify with --branch flag
    char* argv[] = {"gittor", "verify", "--branch", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenCalledWithNoArgs);
    RUN_TEST(shouldPass_whenCalledWithBranchFlag);
    return UNITY_END();
}
