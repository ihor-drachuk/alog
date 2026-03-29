/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <alog/all.h>

namespace {

constexpr int kIterations = 3;

class Barrier {
public:
    explicit Barrier(int count) : m_count(count) {}

    void arrive_and_wait() {
        std::unique_lock<std::mutex> lk(m_mtx);
        if (++m_waiting == m_count) {
            m_released = true;
            m_cv.notify_all();
        } else {
            m_cv.wait(lk, [this]{ return m_released; });
        }
    }

    void reset() {
        m_waiting = 0;
        m_released = false;
    }

private:
    std::mutex m_mtx;
    std::condition_variable m_cv;
    int m_count;
    int m_waiting {};
    bool m_released {};
};

} // namespace


// ---------------------------------------------------------------------------
// 1. Integrity: 100 threads x 1 record, async mode. Compare received == expected as sets.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_Integrity_Async)
{
    constexpr int kThreads = 100;

    for (int iter = 0; iter < kIterations; iter++) {
        std::set<std::string> expected;
        std::set<std::string> received;
        std::mutex receivedMtx;

        for (int i = 0; i < kThreads; i++)
            expected.insert("T" + std::to_string(i));

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record& rec) {
                    std::lock_guard<std::mutex> lk(receivedMtx);
                    received.insert(std::string(rec.getMessage(), rec.getMessageLen()));
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(IntegrityAsync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i = 0; i < kThreads; i++) {
                threads.emplace_back([&, i]() {
                    barrier.arrive_and_wait();
                    auto msg = "T" + std::to_string(i);
                    ACCESS_ALOGGER_MODULE += ALog::Record::create(ALog::Severity::Debug, __LINE__, __FILE__, "", __func__) << msg.c_str();
                });
            }

            for (auto& t : threads)
                t.join();
        } // Logger destroyed here -- flushes async queue

        ASSERT_EQ(received, expected) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 2. Integrity: 100 threads x 1 record, sync mode. Compare received == expected as sets.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_Integrity_Sync)
{
    constexpr int kThreads = 100;

    for (int iter = 0; iter < kIterations; iter++) {
        std::set<std::string> expected;
        std::set<std::string> received;
        std::mutex receivedMtx;

        for (int i = 0; i < kThreads; i++)
            expected.insert("T" + std::to_string(i));

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record& rec) {
                    std::lock_guard<std::mutex> lk(receivedMtx);
                    received.insert(std::string(rec.getMessage(), rec.getMessageLen()));
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(IntegritySync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i = 0; i < kThreads; i++) {
                threads.emplace_back([&, i]() {
                    barrier.arrive_and_wait();
                    auto msg = "T" + std::to_string(i);
                    ACCESS_ALOGGER_MODULE += ALog::Record::create(ALog::Severity::Debug, __LINE__, __FILE__, "", __func__) << msg.c_str();
                });
            }

            for (auto& t : threads)
                t.join();
        }

        ASSERT_EQ(received, expected) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 3. Master becomes available while threads are actively logging.
//    Tests m_masterAvailable transition + markReady vs renew().
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, MasterBecomesAvailable_DuringLogging)
{
    constexpr int kThreads = 16;
    constexpr int kRecordsPerPhase = 50;

    for (int iter = 0; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record&) {
                    recordCount.fetch_add(1, std::memory_order_relaxed);
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            // NOT marking ready yet

            std::atomic<bool> phase2 { false };
            std::vector<std::thread> threads;

            for (int i = 0; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    DEFINE_ALOGGER_MODULE(MasterTransition);

                    // Phase 1: log before master is ready
                    for (int j = 0; j < kRecordsPerPhase; j++)
                        LOGD;

                    // Wait for phase 2
                    while (!phase2.load(std::memory_order_acquire))
                        std::this_thread::yield();

                    // Phase 2: log after master became ready
                    for (int j = 0; j < kRecordsPerPhase; j++)
                        LOGD;
                });
            }

            // Let threads start and queue some records
            std::this_thread::yield();
            std::this_thread::yield();

            MARK_ALOGGER_READY;
            phase2.store(true, std::memory_order_release);

            for (auto& t : threads)
                t.join();
        }

        ASSERT_EQ(recordCount.load(), kThreads * kRecordsPerPhase * 2) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 4. High volume: 32 threads x 50 records, async mode.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_HighVolume_Async)
{
    constexpr int kThreads = 32;
    constexpr int kRecordsPerThread = 50;

    for (int iter = 0; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record&) {
                    recordCount.fetch_add(1, std::memory_order_relaxed);
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(HighVolumeAsync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i = 0; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    barrier.arrive_and_wait();
                    for (int j = 0; j < kRecordsPerThread; j++)
                        LOGD;
                });
            }

            for (auto& t : threads)
                t.join();
        }

        ASSERT_EQ(recordCount.load(), kThreads * kRecordsPerThread) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 5. High volume: 32 threads x 100 records, sync mode.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_HighVolume_Sync)
{
    constexpr int kThreads = 32;
    constexpr int kRecordsPerThread = 100;

    for (int iter = 0; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record&) {
                    recordCount.fetch_add(1, std::memory_order_relaxed);
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(HighVolumeSync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i = 0; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    barrier.arrive_and_wait();
                    for (int j = 0; j < kRecordsPerThread; j++)
                        LOGD;
                });
            }

            for (auto& t : threads)
                t.join();
        }

        ASSERT_EQ(recordCount.load(), kThreads * kRecordsPerThread) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 6. Flush under concurrent writes (async mode). Tests for deadlock and record loss.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, FlushUnderConcurrentWrites)
{
    constexpr int kWriterThreads = 16;
    constexpr int kRecordsPerWriter = 200;
    constexpr int kFlushCount = 20;

    for (int iter = 0; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record&) {
                    recordCount.fetch_add(1, std::memory_order_relaxed);
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(FlushStress);

            std::atomic<bool> start { false };
            std::vector<std::thread> threads;

            // Writer threads
            for (int i = 0; i < kWriterThreads; i++) {
                threads.emplace_back([&]() {
                    while (!start.load(std::memory_order_acquire))
                        std::this_thread::yield();
                    for (int j = 0; j < kRecordsPerWriter; j++)
                        LOGD;
                });
            }

            // Flusher thread
            threads.emplace_back([&]() {
                while (!start.load(std::memory_order_acquire))
                    std::this_thread::yield();
                for (int j = 0; j < kFlushCount; j++)
                    ACCESS_ALOGGER_MODULE.flush();
            });

            start.store(true, std::memory_order_release);

            for (auto& t : threads)
                t.join();
        }

        ASSERT_EQ(recordCount.load(), kWriterThreads * kRecordsPerWriter) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 7. High contention: 64 threads x 1000 records, async mode.
//    Regression guard for future lock-free MPSC queue.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, HighContention_AsyncQueue)
{
    constexpr int kThreads = 64;
    constexpr int kRecordsPerThread = 1000;

    for (int iter = 0; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record&) {
                    recordCount.fetch_add(1, std::memory_order_relaxed);
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(HighContention);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i = 0; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    barrier.arrive_and_wait();
                    for (int j = 0; j < kRecordsPerThread; j++)
                        LOGD;
                });
            }

            for (auto& t : threads)
                t.join();
        }

        ASSERT_EQ(recordCount.load(), kThreads * kRecordsPerThread) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 8. Mixed sync/async mode switch with concurrent logging between switches.
//    Verifies queue drain correctness during mode transitions.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, MixedSyncAsyncModeSwitch)
{
    constexpr int kSwitchCycles = 5;
    constexpr int kThreadsPerBatch = 8;
    constexpr int kRecordsPerThread = 20;

    for (int iter = 0; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto sink = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record&) {
                    recordCount.fetch_add(1, std::memory_order_relaxed);
                });
            ALOGGER_DIRECT->pipeline().sinks().set(sink);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(ModeSwitch);

            for (int cycle = 0; cycle < kSwitchCycles; cycle++) {
                // Async batch
                ALOGGER_DIRECT->setMode(ALog::Logger::AsynchronousSort);
                {
                    std::vector<std::thread> threads;
                    for (int i = 0; i < kThreadsPerBatch; i++) {
                        threads.emplace_back([&]() {
                            for (int j = 0; j < kRecordsPerThread; j++)
                                LOGD;
                        });
                    }
                    for (auto& t : threads)
                        t.join();
                }
                ALOGGER_DIRECT->flush();

                // Sync batch
                ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
                {
                    std::vector<std::thread> threads;
                    for (int i = 0; i < kThreadsPerBatch; i++) {
                        threads.emplace_back([&]() {
                            for (int j = 0; j < kRecordsPerThread; j++)
                                LOGD;
                        });
                    }
                    for (auto& t : threads)
                        t.join();
                }
                ALOGGER_DIRECT->flush();
            }
        }

        ASSERT_EQ(recordCount.load(), kSwitchCycles * 2 * kThreadsPerBatch * kRecordsPerThread) << "Iteration " << iter;
    }
}
