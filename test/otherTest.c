#include "unity/unity.h"

void test_Pass() {
    TEST_ASSERT_EQUAL(40, 40);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_Pass);
    return UNITY_END();
}
