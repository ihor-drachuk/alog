#include <benchmark/benchmark.h>
#include <string>
#include <optional>
#include <memory>
#include "alog/tools.h"


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

#if !defined(ALOG_MACOSX) && !defined(ALOG_LINUX)
static void ALog_Tools_int2str_itoa(benchmark::State& state)
{
    char buffer[10];
    while (state.KeepRunning()) {
        itoa(-812394, buffer, 10);
    }
}

BENCHMARK(ALog_Tools_int2str_itoa);
#endif // !defined(ALOG_MACOSX) && !defined(ALOG_LINUX)


static void ALog_Tools_int2str_sprintf(benchmark::State& state)
{
    char buffer[10];
    while (state.KeepRunning()) {
        sprintf(buffer, "%d", -812394);
    }
}

BENCHMARK(ALog_Tools_int2str_sprintf);


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


static void LongSSO_typical(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ALog::LongSSO<> str;
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
        ALog::LongSSO<> str;
        str.appendString("1234", 4);
        str.appendString(m_str.data());
    }
}


BENCHMARK_F(ALogToolsFixture, LongSSO_long_opt)(benchmark::State& state)
{
    while (state.KeepRunning()) {
        ALog::LongSSO<> str(m_buffer);
        str.appendString("1234", 4);
        str.appendString(m_str.data());
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
        ALog::LongSSO<> str;
        str.appendFmtString("%s %d %5d", "Test", 11, 7);
    }
}

BENCHMARK(LongSSO_typical_fmt);


static void LongSSO_copy(benchmark::State& state)
{
    ALog::LongSSO<> str;
    str.appendString("Test");

    while (state.KeepRunning()) {
        ALog::LongSSO<> str2(str);
    }
}

BENCHMARK(LongSSO_copy);


static void LongSSO_move_x2(benchmark::State& state)
{
    ALog::LongSSO<> str;
    str.appendString("Test");

    while (state.KeepRunning()) {
        ALog::LongSSO<> str2(std::move(str));
        str = std::move(str2);
    }
}

BENCHMARK(LongSSO_move_x2);


BENCHMARK_MAIN();
