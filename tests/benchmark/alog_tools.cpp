/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <benchmark/benchmark.h>
#include <string>
#include <optional>
#include <memory>
#include <variant>
#include <sstream>
#include <numeric>
#include <cinttypes>
#include <alog/tools.h>
#include <alog/tools_jeaiii_to_text.h>

class ALogToolsFixture : public ::benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) {
        m_str = std::string(200, 'a');
    }

    void TearDown(const ::benchmark::State&) {
    }

    std::string m_str;
    ALog::Buffer m_buffer;
};


namespace {

struct SomeResult {
    int a, b, c;
};

enum ErrorCode {
    Ok,
    NotOk
};

SomeResult someResultProvider(int a, int b, int c, ErrorCode& code)
{
    code = Ok;
    return SomeResult{a, b, c};
}

std::variant<SomeResult, ErrorCode> someResultProvider(int a, int b, int c)
{
    return SomeResult{a, b, c};
}

} // namespace

static void ALog_Tools_call_with_ref(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ErrorCode code;
        auto result = someResultProvider(1, 2, 3, code);
        auto isError = (code != Ok);
        const auto& result2 = result;
        (void)result2, (void)isError;
    }
}

BENCHMARK(ALog_Tools_call_with_ref);


static void ALog_Tools_call_with_variant(benchmark::State& state)
{
    while (state.KeepRunning()) {
        auto result = someResultProvider(1, 2, 3);
        auto isError = !std::holds_alternative<SomeResult>(result);
        const auto& result2 = std::get<SomeResult>(result);
        (void)result2, (void)isError;
    }
}

BENCHMARK(ALog_Tools_call_with_variant);


static void ALog_Tools_construct_string(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::string s;
        (void)s;
    }
}

BENCHMARK(ALog_Tools_construct_string);


static void ALog_Tools_construct_optional(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::optional<std::string> s;
        (void)s;
    }
}

BENCHMARK(ALog_Tools_construct_optional);


static void ALog_Tools_construct_shared_ptr(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::shared_ptr<std::string> s;
        (void)s;
    }
}

BENCHMARK(ALog_Tools_construct_shared_ptr);


static void ALog_Tools_construct_unique_ptr(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::unique_ptr<std::string> s;
        (void)s;
    }
}

BENCHMARK(ALog_Tools_construct_unique_ptr);


static void ALog_Tools_optional_bool(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ALog::I::optional_bool opt;
        opt = ((&state) + 1 != nullptr);
        auto result = opt.value_or(false);
        (void)result;
    }
}

BENCHMARK(ALog_Tools_optional_bool);


static void ALog_Tools_optional_bool_std(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::optional<bool> opt;
        opt = ((&state) + 1 != nullptr);
        auto result = opt.value_or(false);
        (void)result;
    }
}

BENCHMARK(ALog_Tools_optional_bool_std);


static void LongSSO_typical(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ALog::I::LongSSO<> str;
        str.appendString("1234", 4);
        str.appendString("1234567890", 10);
        str.appendString("1234", 4);
        str.appendString("1234567890", 10);
    }
}

BENCHMARK(LongSSO_typical);


static void LongSSO_std_typical(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::string str;
        str.append("1234", 4);
        str.append("1234567890", 10);
        str.append("1234", 4);
        str.append("1234567890", 10);
    }
}

BENCHMARK(LongSSO_std_typical);


BENCHMARK_F(ALogToolsFixture, LongSSO_long)(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ALog::I::LongSSO<> str;
        str.appendString("1234", 4);
        str.appendString(m_str.data(), m_str.size());
    }
}


BENCHMARK_F(ALogToolsFixture, LongSSO_long_opt)(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ALog::I::LongSSO<> str(m_buffer);
        str.appendString("1234", 4);
        str.appendString(m_str.data(), m_str.size());
    }
}


BENCHMARK_F(ALogToolsFixture, LongSSO_std_long)(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::string str;
        str.append("1234", 4);
        str.append(m_str.data());
    }
}


static void LongSSO_typical_fmt(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ALog::I::LongSSO<> str;
        str.appendFmtString("%s %d %5d", "Test", 11, 7);
    }
}

BENCHMARK(LongSSO_typical_fmt);


static void LongSSO_copy(benchmark::State& state)
{
    ALog::I::LongSSO<> str;
    str.appendString("Test");

    while (state.KeepRunning()) {
        ALog::I::LongSSO<> str2(str);
    }
}

BENCHMARK(LongSSO_copy);


static void LongSSO_move_x2(benchmark::State& state)
{
    ALog::I::LongSSO<> str;
    str.appendString("Test");

    while (state.KeepRunning()) {
        ALog::I::LongSSO<> str2(std::move(str));
        str = std::move(str2);
    }
}

BENCHMARK(LongSSO_move_x2);

static inline void touchChar(char c) { (void)c; }

static void LoopDirection_bkwd(benchmark::State& state)
{
    std::string s(1024, 'a');

    while (state.KeepRunning()) {
        for (int i = s.size() - 1; i >= 0; i--)
            touchChar(s.at(i));
    }
}

BENCHMARK(LoopDirection_bkwd);

static void LoopDirection_bkwd_rev_it(benchmark::State& state)
{
    std::string s(1024, 'a');

    while (state.KeepRunning()) {
        for (auto it = s.crbegin(), end = s.crend(); it != end; it++)
            touchChar(*it);
    }
}

BENCHMARK(LoopDirection_bkwd_rev_it);

static void LoopDirection_fwd(benchmark::State& state)
{
    std::string s(1024, 'a');

    while (state.KeepRunning()) {
        for (int i = 0; i < s.size(); i++)
            touchChar(s.at(i));
    }
}

BENCHMARK(LoopDirection_fwd);

static void LoopDirection_fwd_s(benchmark::State& state)
{
    std::string s(1024, 'a');

    while (state.KeepRunning()) {
        for (int i = 0, sz = s.size(); i < sz; i++)
            touchChar(s.at(i));
    }
}

BENCHMARK(LoopDirection_fwd_s);

static void LoopDirection_fwd_it(benchmark::State& state)
{
    std::string s(1024, 'a');

    while (state.KeepRunning()) {
        for (auto it = s.cbegin(); it != s.cend(); it++)
            touchChar(*it);
    }
}

BENCHMARK(LoopDirection_fwd_it);

static void LoopDirection_fwd_it_s(benchmark::State& state)
{
    std::string s(1024, 'a');

    while (state.KeepRunning()) {
        for (auto it = s.cbegin(), end = s.cend(); it != end; it++)
            touchChar(*it);
    }
}

BENCHMARK(LoopDirection_fwd_it_s);

static void LoopDirection_foreach(benchmark::State& state)
{
    std::string s(1024, 'a');

    while (state.KeepRunning()) {
        for (const auto& x : s)
            touchChar(x);
    }
}

BENCHMARK(LoopDirection_foreach);


static void IntToStr_sstream(benchmark::State& state)
{
    while (state.KeepRunning()) {
        std::stringstream ss;
        ss << std::numeric_limits<uint16_t>::max();
        (void)ss.str();
    }
}

BENCHMARK(IntToStr_sstream);


static void IntToStr_sprintf(benchmark::State& state)
{
    while (state.KeepRunning()) {
        char buffer[std::numeric_limits<uint16_t>::digits+2];
        sprintf(buffer, "%" PRIu16, std::numeric_limits<uint16_t>::max());
        (void)buffer;
    }
}

BENCHMARK(IntToStr_sprintf);


#if !defined(ALOG_OS_MACOS) && !defined(ALOG_OS_LINUX)
static void IntToStr_itoa(benchmark::State& state)
{
    while (state.KeepRunning()) {
        char buffer[std::numeric_limits<uint16_t>::digits+2];
        itoa(std::numeric_limits<uint16_t>::max(), buffer, 10);
        (void)buffer;
    }
}

BENCHMARK(IntToStr_itoa);
#endif // !defined(ALOG_OS_MACOS) && !defined(ALOG_OS_LINUX)


static void IntToStr_jeaiii(benchmark::State& state)
{
    while (state.KeepRunning()) {
        char buffer[std::numeric_limits<uint16_t>::digits+2];
        *(jeaiii::to_text_from_integer(buffer, std::numeric_limits<uint16_t>::max())) = 0;
        (void)buffer;
    }
}

BENCHMARK(IntToStr_jeaiii);

BENCHMARK_MAIN();
