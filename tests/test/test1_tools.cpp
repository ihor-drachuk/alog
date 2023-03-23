#include <gtest/gtest.h>
#include <alog/tools.h>
#include <alog/tools_internal.h>
#include <vector>
#include <string>
#include <cassert>

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
        auto _f2 (std::move(_f));
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

TEST(ALog_Tools, AnalyzePath)
{
    const std::vector<std::string> inputs {
        "C:\\file.txt",
        "C:\\dir\\file.txt",
        "C:\\dir.smth\\file",
        "/file.txt",
        "file.txt",
        "/some.dir/file.txt",
        "file",
        "dir/subdir/file.txt",
        "dir/.txt",
        ".txt",
        ""
    };

    const std::vector<ALog::Internal::FilePathDetails> results {
        {"C:\\", "file", ".txt"},
        {"C:\\dir\\", "file", ".txt"},
        {"C:\\dir.smth\\", "file", ""},
        {"/", "file", ".txt"},
        {"", "file", ".txt"},
        {"/some.dir/", "file", ".txt"},
        {"", "file", ""},
        {"dir/subdir/", "file", ".txt"},
        {"dir/", "", ".txt"},
        {"", "", ".txt"},
        {}
    };

    assert(inputs.size() == results.size());

    for (int i = 0; i < inputs.size(); i++) {
        const auto actualResult = ALog::Internal::analyzePath(inputs.at(i));
        ASSERT_EQ(actualResult, results.at(i));
    }
}
