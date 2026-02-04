#include "api/api.h"
#include "unity/unity.h"

static void shouldPass_whenHeartbeatToValidEndpoint() {
    // GIVEN: A valid endpoint URL
    const char* endpoint_url = "https://gittor.rent/api/";

    // WHEN: Call heartbeat
    int err = heartbeat(endpoint_url);

    // THEN: Should return 0 error
    TEST_ASSERT_EQUAL(0, err);
}

static void shouldFail_whenHeartbeatToInvalidEndpoint() {
    // GIVEN: An invalid endpoint URL
    const char* endpoint_url = "http://invalid.endpoint.local/api/";

    // WHEN: Call heartbeat
    int err = heartbeat(endpoint_url);

    // THEN: Should return non-zero error
    TEST_ASSERT_NOT_EQUAL(0, err);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(shouldPass_whenHeartbeatToValidEndpoint);
    RUN_TEST(shouldFail_whenHeartbeatToInvalidEndpoint);
    return UNITY_END();
}
