#include <benchmark/benchmark.h>
#include "alog/logger.h"

static void LogMessage_module(benchmark::State& state)
{
    DEFINE_ALOGGER_MODULE(ALogTest);

    while (state.KeepRunning())
        LOGD;
}

BENCHMARK(LogMessage_module);


static void LogMessage_module_and_main(benchmark::State& state)
{
    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT.markReady();
    DEFINE_ALOGGER_MODULE(ALogTest);

    while (state.KeepRunning())
        LOGD;
}

BENCHMARK(LogMessage_module_and_main);


static void LogMessage_main_async(benchmark::State& state)
{
    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->setMode(ALog::Logger::Asynchronous);
    ALOGGER_DIRECT.markReady();

    while (state.KeepRunning())
        LOGMD;
}

BENCHMARK(LogMessage_main_async);


static void LogMessage_main_sync(benchmark::State& state)
{
    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    ALOGGER_DIRECT.markReady();

    while (state.KeepRunning())
        LOGMD;
}

BENCHMARK(LogMessage_main_sync);


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
