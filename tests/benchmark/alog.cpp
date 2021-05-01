#include <benchmark/benchmark.h>
#include "alog/logger.h"

static void LogMessage(benchmark::State& state)
{
    DEFINE_ALOGGER_MODULE(ALogTest);

    while (state.KeepRunning())
        LOGD;
}

BENCHMARK(LogMessage);


static void LogMessage_complex(benchmark::State& state)
{
    DEFINE_ALOGGER_MODULE(ALogTest);

    auto d = std::vector<std::vector<std::pair<int, const char*>>> {
        {{1, "1"}, {2, "2"}, {3, "3"}},
        {{4, "4"}, {5, "5"}},
    };

    while (state.KeepRunning())
        LOGD << d;
}

BENCHMARK(LogMessage_complex);


BENCHMARK_MAIN();
