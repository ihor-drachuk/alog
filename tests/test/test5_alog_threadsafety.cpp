/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <future>
#include <initializer_list>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <thread>
#include <vector>

#include <alog/all.h>

namespace {

constexpr int kIterations {3};

class Barrier {
public:
    Barrier(int count)
        : m_count(count)
    {}

    void arrive_and_wait() {
        std::unique_lock<std::mutex> waitLock(m_waitMutex);
        if (++m_waiting == m_count) {
            m_released = true;
            m_waitCv.notify_all();
        } else {
            m_waitCv.wait(waitLock, [this]{ return m_released; });
        }
    }

    void reset() {
        m_waiting = 0;
        m_released = false;
    }

private:
    std::mutex m_waitMutex;
    std::condition_variable m_waitCv;
    int m_count;
    int m_waiting {};
    bool m_released {};
};

} // namespace


// ---------------------------------------------------------------------------
// 1. Integrity: 100 threads x 1 record, async mode. Compare `receivedMessages` and `expectedMessages` as sets.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_Integrity_Async)
{
    constexpr int kThreads {100};

    for (int iter {}; iter < kIterations; iter++) {
        std::set<std::string> expectedMessages;
        std::set<std::string> receivedMessages;
        std::mutex receivedMutex;

        for (int i {}; i < kThreads; i++)
            expectedMessages.insert("T" + std::to_string(i));

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record& record) {
                    std::lock_guard<std::mutex> lk(receivedMutex);
                    receivedMessages.insert(std::string(record.getMessage(), record.getMessageLen()));
                });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(IntegrityAsync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i {}; i < kThreads; i++) {
                threads.emplace_back([&, i]() {
                    barrier.arrive_and_wait();
                    auto msg = "T" + std::to_string(i);
                    ACCESS_ALOGGER_MODULE +=
                        ALog::Record::create(ALog::Severity::Debug, __LINE__, __FILE__, "", __func__) << msg.c_str();
                });
            }

            for (auto& workerThread : threads)
                workerThread.join();
        } // Logger destroyed here -- flushes async queue

        ASSERT_EQ(receivedMessages, expectedMessages) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 2. Integrity: 100 threads x 1 record, sync mode. Compare `receivedMessages` and `expectedMessages` as sets.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_Integrity_Sync)
{
    constexpr int kThreads {100};

    for (int iter {}; iter < kIterations; iter++) {
        std::set<std::string> expectedMessages;
        std::set<std::string> receivedMessages;
        std::mutex receivedMutex;

        for (int i {}; i < kThreads; i++)
            expectedMessages.insert("T" + std::to_string(i));

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record& record) {
                    std::lock_guard<std::mutex> lk(receivedMutex);
                    receivedMessages.insert(std::string(record.getMessage(), record.getMessageLen()));
                });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(IntegritySync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i {}; i < kThreads; i++) {
                threads.emplace_back([&, i]() {
                    barrier.arrive_and_wait();
                    auto msg = "T" + std::to_string(i);
                    ACCESS_ALOGGER_MODULE +=
                        ALog::Record::create(ALog::Severity::Debug, __LINE__, __FILE__, "", __func__) << msg.c_str();
                });
            }

            for (auto& workerThread : threads)
                workerThread.join();
        }

        ASSERT_EQ(receivedMessages, expectedMessages) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 3. Master becomes available while threads are actively logging.
//    Tests `m_masterAvailable` transition + `markReady` vs `renew()`.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, MasterBecomesAvailable_DuringLogging)
{
    constexpr int kThreads {16};
    constexpr int kRecordsPerPhase {50};

    for (int iter {}; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>([&](const ALog::Buffer&, const ALog::Record&) {
                recordCount.fetch_add(1, std::memory_order_relaxed);
            });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            // NOT marking ready yet

            std::atomic<bool> phase2 {};
            std::vector<std::thread> threads;

            for (int i {}; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    DEFINE_ALOGGER_MODULE(MasterTransition);

                    // Phase 1: log before master is ready
                    for (int j {}; j < kRecordsPerPhase; j++)
                        LOGD;

                    // Wait for phase 2
                    while (!phase2.load(std::memory_order_acquire))
                        std::this_thread::yield();

                    // Phase 2: log after master became ready
                    for (int j {}; j < kRecordsPerPhase; j++)
                        LOGD;
                });
            }

            // Let threads start and queue some records
            std::this_thread::yield();
            std::this_thread::yield();

            MARK_ALOGGER_READY;
            phase2.store(true, std::memory_order_release);

            for (auto& workerThread : threads)
                workerThread.join();
        }

        ASSERT_EQ(recordCount.load(), kThreads * kRecordsPerPhase * 2) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 4. High volume: 32 threads x 50 records, async mode.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_HighVolume_Async)
{
    constexpr int kThreads {32};
    constexpr int kRecordsPerThread {50};

    for (int iter {}; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>([&](const ALog::Buffer&, const ALog::Record&) {
                recordCount.fetch_add(1, std::memory_order_relaxed);
            });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(HighVolumeAsync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i {}; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    barrier.arrive_and_wait();
                    for (int j {}; j < kRecordsPerThread; j++)
                        LOGD;
                });
            }

            for (auto& workerThread : threads)
                workerThread.join();
        }

        ASSERT_EQ(recordCount.load(), kThreads * kRecordsPerThread) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 5. High volume: 32 threads x 100 records, sync mode.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentLog_HighVolume_Sync)
{
    constexpr int kThreads {32};
    constexpr int kRecordsPerThread {100};

    for (int iter {}; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>([&](const ALog::Buffer&, const ALog::Record&) {
                recordCount.fetch_add(1, std::memory_order_relaxed);
            });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(HighVolumeSync);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i {}; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    barrier.arrive_and_wait();
                    for (int j {}; j < kRecordsPerThread; j++)
                        LOGD;
                });
            }

            for (auto& workerThread : threads)
                workerThread.join();
        }

        ASSERT_EQ(recordCount.load(), kThreads * kRecordsPerThread) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 6. Flush under concurrent writes (async mode). Tests for deadlock and record loss.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, FlushUnderConcurrentWrites)
{
    constexpr int kWriterThreads {16};
    constexpr int kRecordsPerWriter {200};
    constexpr int kFlushCount {20};

    for (int iter {}; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>([&](const ALog::Buffer&, const ALog::Record&) {
                recordCount.fetch_add(1, std::memory_order_relaxed);
            });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(FlushStress);

            std::atomic<bool> start {};
            std::vector<std::thread> threads;

            // Writer threads
            for (int i {}; i < kWriterThreads; i++) {
                threads.emplace_back([&]() {
                    while (!start.load(std::memory_order_acquire))
                        std::this_thread::yield();
                    for (int j {}; j < kRecordsPerWriter; j++)
                        LOGD;
                });
            }

            // Flusher thread
            threads.emplace_back([&]() {
                while (!start.load(std::memory_order_acquire))
                    std::this_thread::yield();
                for (int j {}; j < kFlushCount; j++)
                    ACCESS_ALOGGER_MODULE.flush();
            });

            start.store(true, std::memory_order_release);

            for (auto& workerThread : threads)
                workerThread.join();
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
    constexpr int kThreads {64};
    constexpr int kRecordsPerThread {1000};

    for (int iter {}; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>([&](const ALog::Buffer&, const ALog::Record&) {
                recordCount.fetch_add(1, std::memory_order_relaxed);
            });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(HighContention);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i {}; i < kThreads; i++) {
                threads.emplace_back([&]() {
                    barrier.arrive_and_wait();
                    for (int j {}; j < kRecordsPerThread; j++)
                        LOGD;
                });
            }

            for (auto& workerThread : threads)
                workerThread.join();
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
    constexpr int kSwitchCycles {5};
    constexpr int kThreadsPerBatch {8};
    constexpr int kRecordsPerThread {20};

    for (int iter {}; iter < kIterations; iter++) {
        std::atomic<int> recordCount {};

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>([&](const ALog::Buffer&, const ALog::Record&) {
                recordCount.fetch_add(1, std::memory_order_relaxed);
            });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(ModeSwitch);

            for (int cycle {}; cycle < kSwitchCycles; cycle++) {
                // Async batch
                ALOGGER_DIRECT->setMode(ALog::Logger::AsynchronousSort);
                {
                    std::vector<std::thread> threads;
                    for (int i {}; i < kThreadsPerBatch; i++) {
                        threads.emplace_back([&]() {
                            for (int j {}; j < kRecordsPerThread; j++)
                                LOGD;
                        });
                    }
                    for (auto& workerThread : threads)
                        workerThread.join();
                }
                ALOGGER_DIRECT->flush();

                // Sync batch
                ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
                {
                    std::vector<std::thread> threads;
                    for (int i {}; i < kThreadsPerBatch; i++) {
                        threads.emplace_back([&]() {
                            for (int j {}; j < kRecordsPerThread; j++)
                                LOGD;
                        });
                    }
                    for (auto& workerThread : threads)
                        workerThread.join();
                }
                ALOGGER_DIRECT->flush();
            }
        }

        ASSERT_EQ(recordCount.load(), kSwitchCycles * 2 * kThreadsPerBatch * kRecordsPerThread) << "Iteration " << iter;
    }
}


namespace {

class EventSink : public ALog::ISink
{
public:
    void write(const ALog::Buffer&, const ALog::Record& record) override {
        if (m_blockNext.exchange(false)) {
            m_blockedPromise.set_value();
            m_gatePromise.get_future().wait();
        }

        std::lock_guard<std::mutex> lk(m_eventsMutex);
        m_events.emplace_back(record.getMessage(), record.getMessageLen());
    }

    void flush() override {
        // We sleep here, so the test fails when `Logger::flush()` returns before the sinks are flushed.
        std::this_thread::sleep_for(std::chrono::milliseconds(50));

        std::lock_guard<std::mutex> lk(m_eventsMutex);
        m_events.emplace_back("<flush>");
    }

    // Must not be called while `threadFunc()` is inside `write()`.
    void blockNextWrite() {
        m_blockedPromise = std::promise<void>();
        m_gatePromise = std::promise<void>();
        m_blockNext = true;
    }

    void waitBlocked() { m_blockedPromise.get_future().wait(); }
    void openGate() { m_gatePromise.set_value(); }

    std::vector<std::string> events() {
        std::lock_guard<std::mutex> lk(m_eventsMutex);
        return m_events;
    }

private:
    std::mutex m_eventsMutex;
    std::vector<std::string> m_events;
    std::atomic<bool> m_blockNext {};
    std::promise<void> m_blockedPromise;
    std::promise<void> m_gatePromise;
};

} // namespace


// ---------------------------------------------------------------------------
// 9. Many threads wait for flush via `FLUSH` and `flush()`. `LOGD << FLUSH` returns after its record is written.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, ConcurrentFlushWaiters)
{
    constexpr int kThreads {8};
    constexpr int kRecordsPerThread {500};

    for (const auto mode : {ALog::Logger::Asynchronous, ALog::Logger::AsynchronousSort}) {
        for (int iter {}; iter < kIterations; iter++) {
            std::vector<std::atomic<bool>> written(kThreads * kRecordsPerThread);
            std::atomic<int> notWrittenOnReturn {};

            {
                DEFINE_MAIN_ALOGGER;
                auto functor2 = std::make_shared<ALog::Sinks::Functor2>(
                    [&](const ALog::Buffer&, const ALog::Record& record) {
                        written[std::stoi(std::string(record.getMessage(), record.getMessageLen()))].store(true);
                    });
                ALOGGER_DIRECT->pipeline().sinks().set(functor2);
                ALOGGER_DIRECT->setMode(mode);
                MARK_ALOGGER_READY;

                DEFINE_ALOGGER_MODULE(ConcurrentFlushWaiters);

                Barrier barrier(kThreads);
                std::vector<std::thread> threads;

                for (int i {}; i < kThreads; i++) {
                    threads.emplace_back([&, i]() {
                        barrier.arrive_and_wait();
                        for (int j {}; j < kRecordsPerThread; j++) {
                            const int id {i * kRecordsPerThread + j};
                            LOGD << id << FLUSH;

                            if (!written[id].load())
                                notWrittenOnReturn++;

                            if (j % 50 == 0)
                                ACCESS_ALOGGER_MODULE.flush();
                        }
                    });
                }

                for (auto& workerThread : threads)
                    workerThread.join();
            }

            ASSERT_EQ(notWrittenOnReturn.load(), 0) << "Mode " << mode << ", iteration " << iter;
        }
    }
}


// ---------------------------------------------------------------------------
// 10. Autoflush with concurrent writers. `LOGD` returns after its record is written.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, Autoflush_ConcurrentWriters)
{
    constexpr int kThreads {8};
    constexpr int kRecordsPerThread {300};

    for (int iter {}; iter < kIterations; iter++) {
        std::vector<std::atomic<bool>> written(kThreads * kRecordsPerThread);
        std::atomic<int> notWrittenOnReturn {};

        {
            DEFINE_MAIN_ALOGGER;
            auto functor2 = std::make_shared<ALog::Sinks::Functor2>(
                [&](const ALog::Buffer&, const ALog::Record& record) {
                    written[std::stoi(std::string(record.getMessage(), record.getMessageLen()))].store(true);
                });
            ALOGGER_DIRECT->pipeline().sinks().set(functor2);
            ALOGGER_DIRECT->setAutoflush(true);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(AutoflushConcurrent);

            Barrier barrier(kThreads);
            std::vector<std::thread> threads;

            for (int i {}; i < kThreads; i++) {
                threads.emplace_back([&, i]() {
                    barrier.arrive_and_wait();
                    for (int j {}; j < kRecordsPerThread; j++) {
                        const int id {i * kRecordsPerThread + j};
                        LOGD << id;

                        if (!written[id].load())
                            notWrittenOnReturn++;
                    }
                });
            }

            for (auto& workerThread : threads)
                workerThread.join();
        }

        ASSERT_EQ(notWrittenOnReturn.load(), 0) << "Iteration " << iter;
    }
}


// ---------------------------------------------------------------------------
// 11. `flush()` flushes sinks after earlier records and before it returns. A batch without `Flush` does not flush.
// ---------------------------------------------------------------------------
TEST(ALog_ThreadSafety, FlushAfterQueuedRecords)
{
    for (const auto mode : {ALog::Logger::Asynchronous, ALog::Logger::AsynchronousSort}) {
        for (int useModuleFlush {}; useModuleFlush < 2; useModuleFlush++) {
            DEFINE_MAIN_ALOGGER;
            auto eventSink = std::make_shared<EventSink>();
            ALOGGER_DIRECT->pipeline().sinks().set(eventSink);
            ALOGGER_DIRECT->setMode(mode);
            MARK_ALOGGER_READY;

            DEFINE_ALOGGER_MODULE(FlushOrder);

            // `flush()` does not return until `openGate()` is called, so we call `openGate()` from `openerThread`.
            // Nothing signals that `flush()` has queued its record, so we wait 50 ms before `openGate()`.
            const auto flushAndReadEvents = [&]() {
                std::thread openerThread([&]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    eventSink->openGate();
                });

                if (useModuleFlush) {
                    ACCESS_ALOGGER_MODULE.flush();
                } else {
                    ALOGGER_DIRECT->flush();
                }

                auto events = eventSink->events();
                openerThread.join();
                return events;
            };

            // We block `threadFunc()` while it writes "blocker", so "a", "b" and the `flush()` record form one batch.
            eventSink->blockNextWrite();
            LOGD << "blocker";
            eventSink->waitBlocked();
            LOGD << "a";
            LOGD << "b";
            const auto firstEvents = flushAndReadEvents();

            eventSink->blockNextWrite();
            LOGD << "blocker2";
            eventSink->waitBlocked();
            LOGD << "c";
            const auto secondEvents = flushAndReadEvents();

            const std::vector<std::string> firstExpected {"blocker", "a", "b", "<flush>"};
            const std::vector<std::string> secondExpected {"blocker", "a", "b", "<flush>", "blocker2", "c", "<flush>"};
            ASSERT_EQ(firstEvents, firstExpected) << "Mode " << mode << ", useModuleFlush = " << useModuleFlush;
            ASSERT_EQ(secondEvents, secondExpected) << "Mode " << mode << ", useModuleFlush = " << useModuleFlush;
        }
    }
}
