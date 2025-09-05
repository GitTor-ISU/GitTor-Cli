#include "calc/calc.h"
#include "unity/unity.h"

void test_Pass() {
    TEST_ASSERT_EQUAL(40, add(13, 27));
}

void test_Ignore() {
    TEST_IGNORE();
    TEST_ASSERT_EQUAL(40, add(13, 37));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_Pass);
    RUN_TEST(test_Ignore);
    return UNITY_END();
}
