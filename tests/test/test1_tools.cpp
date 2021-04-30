#include <gtest/gtest.h>
#include "alog/tools.h"

TEST(ALog_Tools, extractFileNameOnly)
{
    constexpr auto result = ALog::extractFileNameOnly("H:\\folder\\sub-folder\\file.cpp");
    ASSERT_STREQ(result, "file.cpp");

    constexpr auto result2 = ALog::extractFileNameOnly("/folder/sub-folder/uncomfortable - file.cpp");
    ASSERT_STREQ(result2, "uncomfortable - file.cpp");
}

TEST(ALog_Tools, Finally)
{
    bool triggered = false;

    {
        auto _f = ALog::CreateFinally([&](){ triggered = true; });
        ASSERT_FALSE(triggered);
    }

    ASSERT_TRUE(triggered);
}
