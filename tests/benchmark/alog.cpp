#include <benchmark/benchmark.h>
#include "alog/logger.h"

DEFINE_ALOGGER_MODULE(ALogTest);

class ALogFixture : public ::benchmark::Fixture {
public:
    void SetUp(const ::benchmark::State&) {
        logger = std::make_unique<ALog::DefaultLogger>();
        (*logger)->setSink(std::make_shared<ALog::SinkNull>());
        (*logger)->setFilter(std::make_shared<ALog::FilterSeverity>(ALog::Severity::Minimal));
        logger->markReady();
    }

    void TearDown(const ::benchmark::State&) {
    }

private:
    std::unique_ptr<ALog::DefaultLogger> logger;
};

static void LogMessage(benchmark::State& state)
{
    while (state.KeepRunning())
        LOGD;
}

BENCHMARK(LogMessage);

BENCHMARK_MAIN();
