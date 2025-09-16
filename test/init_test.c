#include <errno.h>
#include <unistd.h>
#include "cmd/cmd.h"
#include "unity/unity.h"
#include "utils/utils.h"

static void shouldPass_whenNameWithArg() {
    // GIVEN: Name with argument
    char* argv[] = {"gittor", "init", "--name", "repoName", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldPass_whenNameWithoutArg() {
    // GIVEN: Name without argument
    char* argv[] = {"gittor", "init", "--name", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(EINVAL, err);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenNameWithArg);
    RUN_TEST(shouldPass_whenNameWithoutArg);
    return UNITY_END();
}
