#include <errno.h>
#include "cmd/cmd.h"
#include "unity/unity.h"

static void shouldPass_whenCalledWithNoArgs() {
    // GIVEN: Just calling gittor config
    char* argv[] = {"gittor", "devs", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 (success for stub)
    TEST_ASSERT_EQUAL(0, err);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenCalledWithNoArgs);
    return UNITY_END();
}
