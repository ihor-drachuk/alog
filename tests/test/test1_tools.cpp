#include <gtest/gtest.h>
#include <alog/tools.h>
#include <alog/tools_internal.h>
#include <alog/tools_jeaiii_to_text.h>
#include <vector>
#include <string>
#include <cassert>
#include <numeric>
#include <sstream>

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

TEST(ALog_Tools, IntToStr)
{
    auto nativeIntToStr = [](auto value){
        std::stringstream ss;
        ss << value;
        return ss.str();
    };

    auto fastIntToStr = [](auto value) {
        char buffer[std::numeric_limits<decltype(value)>::digits + 2];
        char* const end = jeaiii::to_text_from_integer(buffer, value);
        return std::string(buffer, end);
    };

    auto makeDataPair = [&](auto value) {
        auto nativeResult = nativeIntToStr(value);
        auto fastResult = fastIntToStr(value);
        return std::pair(nativeResult, fastResult);
    };

    auto verifier = [](const std::pair<std::string, std::string>& pair) {
        ASSERT_EQ(pair.first, pair.second);
    };

    const auto datasetSource = std::make_tuple(
        -1, 0, 1,
         10,  100,  1000,  10001,
        -10, -100, -1000, -10001,
        34623,
        -87123,
        std::numeric_limits<int64_t>::min(),
        std::numeric_limits<int64_t>::max(),
        std::numeric_limits<uint64_t>::min(),
        std::numeric_limits<uint64_t>::max()
    );

    const auto dataset = std::apply([&](auto... xs){ return std::make_tuple(makeDataPair(xs)...); }, datasetSource);
    std::apply([&](auto... xs){ (verifier(xs), ...); }, dataset);
}
