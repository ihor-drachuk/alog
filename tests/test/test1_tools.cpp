#include <gtest/gtest.h>
#include "alog/tools.h"

TEST(ALog_Tools, extractFileNameOnly)
{
    constexpr auto result = ALog::I::extractFileNameOnly("H:\\folder\\sub-folder\\file.cpp");
    ASSERT_STREQ(result, "file.cpp");

    constexpr auto result2 = ALog::I::extractFileNameOnly("/folder/sub-folder/uncomfortable - file.cpp");
    ASSERT_STREQ(result2, "uncomfortable - file.cpp");
}

TEST(ALog_Tools, Finally)
{
    bool triggered = false;

    {
        auto _f = ALog::I::CreateFinally([&](){ triggered = true; });
        ASSERT_FALSE(triggered);
    }

    ASSERT_TRUE(triggered);
}

TEST(ALog_Tools, CombineInt)
{
    enum class SE {
        A = 1,
        B = 2,
        C = 4,
        D = 8
    };

    ASSERT_EQ(ALog::I::combineInt(SE::A), 1);
    ASSERT_EQ(ALog::I::combineInt(SE::A, SE::C), 5);
    ASSERT_EQ(ALog::I::combineInt(SE::A, SE::B, SE::C, SE::D), 15);
}
