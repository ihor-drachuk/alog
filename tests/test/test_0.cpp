#include "gtest/gtest.h"

static int add(int a, int b) {
    return a+b;
}

TEST(add, test0)
{
    ASSERT_EQ(add(10, 13), 23);
}
