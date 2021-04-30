#include <gtest/gtest.h>
#include "alog/tools.h"

TEST(ALog_Tools, test0)
{
    constexpr auto result = ALog::extractFileNameOnly("H:\\folder\\sub-folder\\file.cpp");
    ASSERT_STREQ(result, "file.cpp");

    constexpr auto result2 = ALog::extractFileNameOnly("/folder/sub-folder/uncomfortable - file.cpp");
    ASSERT_STREQ(result2, "uncomfortable - file.cpp");
}
