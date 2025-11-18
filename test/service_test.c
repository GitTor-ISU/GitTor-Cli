#include <errno.h>
#include <glib.h>
#include <unistd.h>
#include "cmd/cmd.h"
#include "service/service.h"
#include "service/service_internals.h"
#include "unity/unity.h"
#include "utils/utils.h"

__attribute__((__unused__)) static void shouldEsrch_whenUnknownCommand() {
    // GIVEN: Unknown sub-command call
    char* argv[] = {"gittor", "service", "unknown", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return no such process error
    TEST_ASSERT_EQUAL(ESRCH, err);
}

__attribute__((__unused__)) static void shouldBeDown_whenGetStatus() {
    TEST_ASSERT_EQUAL_STRING(gittor_service_status(), "down");
}

__attribute__((__unused__)) static void shouldBeUp_whenGetStatus() {
    TEST_ASSERT_EQUAL_STRING(gittor_service_status(), "up");
}

__attribute__((__unused__)) static void shouldPass_whenServiceStatus() {
    // GIVEN: call status
    char* argv[] = {"gittor", "service", "status", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

__attribute__((__unused__)) static void shouldPass_whenServicePing() {
    // GIVEN: call ping
    char* argv[] = {"gittor", "service", "ping", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

__attribute__((__unused__)) static void shouldPass_whenServiceStart() {
    // GIVEN: call start
    char* argv[] = {"gittor", "service", "start", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

__attribute__((__unused__)) static void shouldPass_whenServiceStop() {
    // GIVEN: call stop
    char* argv[] = {"gittor", "service", "stop", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

__attribute__((__unused__)) static void shouldPass_whenServiceRestart() {
    // GIVEN: call start
    char* argv[] = {"gittor", "service", "restart", NULL};
    int argc = sizeof(argv) / sizeof(*argv) - 1;

    // WHEN: Parse arguments
    int err = cmd_parse(argc, argv);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

int main() {
    UNITY_BEGIN();

    // Honestly I'm just doing a bunch of random stuff to up coverage and ensure
    // no memory errors
    RUN_TEST(shouldEsrch_whenUnknownCommand);
    RUN_TEST(shouldBeDown_whenGetStatus);
    RUN_TEST(shouldPass_whenServicePing);
    RUN_TEST(shouldBeUp_whenGetStatus);
    gittor_service_disconnect();
    gittor_service_disconnect();
    RUN_TEST(shouldPass_whenServicePing);
    RUN_TEST(shouldPass_whenServiceStop);
    RUN_TEST(shouldPass_whenServiceRestart);
    RUN_TEST(shouldPass_whenServiceRestart);

    return UNITY_END();
}
