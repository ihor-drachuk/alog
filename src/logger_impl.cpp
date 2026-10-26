/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/logger_impl.h>

#include <alog/formatters/default.h>
#include <alog/sinks/console.h>

#include <algorithm>
#include <atomic>
#include <cassert>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#ifdef WIN32
#include <Windows.h>
#define ALOG_STOP_DEBUGGER DebugBreak()
#else
#include <csignal>
#define ALOG_STOP_DEBUGGER raise(SIGTRAP)
#endif // WIN32

namespace ALog {

void alog_breakpoint()
{
    (void)ALOG_STOP_DEBUGGER;
}

void alog_abort()
{
    alog_breakpoint();
    std::abort();
}

void alog_exception(const char* msg)
{
    throw std::runtime_error(msg);
}

void alog_exception(const char* msg, size_t sz)
{
    throw std::runtime_error(std::string(msg, sz));
}


struct Logger::impl_t
{
    ALog::Sinks::Pipeline pipeline;

    LoggerMode mode {};
    std::mutex writeMutex;
    std::mutex queueMutex;
    std::thread workerThread;
    std::condition_variable queueCv;
    std::vector<Record> queue = [] { std::vector<Record> v; v.reserve(256); return v; }();
    bool exitFlag {};
    bool threadRunning {};

    // Guarded by `queueMutex`.
    // Each queued `Flush` record gets the next ticket, and `addRecord` waits until `flushTicketsDone` reaches it,
    // so we raise `flushTicketsDone` in `threadFunc()` only after the sinks are flushed.
    uint64_t flushTicketsIssued {};
    uint64_t flushTicketsDone {};
    std::condition_variable flushCv;

    std::atomic<bool> autoflush {};

    std::chrono::time_point<std::chrono::steady_clock> startTp = std::chrono::steady_clock::now();
};

// `flushTicketsIssued` and `flushTicketsDone` may overflow back to 0, so we compare their difference instead of `>=`.
static bool isFlushTicketDone(uint64_t ticketsDone, uint64_t ticket)
{
    return ticketsDone - ticket < (uint64_t {1} << 63);
}

Logger::Logger()
{
    createImpl();
    setMode(AsynchronousSort);
    setupDefaultConfig();
}

Logger::~Logger()
{
    flush();
    stopThread();
}

void Logger::setupDefaultConfig()
{
    impl().pipeline.reset();
    impl().pipeline.formatter() = std::make_shared<ALog::Formatters::Default>();
    impl().pipeline.sinks().set(std::make_shared<ALog::Sinks::Console>());
}

void Logger::addRecord(Record&& record)
{
    record.startTp = impl().startTp;

    if (record.steadyTp < record.startTp)
        record.steadyTp = record.startTp;

    if (impl().autoflush.load(std::memory_order_relaxed))
        record.flagsOn(Record::Flags::Flush);

    if (impl().mode == Synchronous) {
        // Sync write
        std::unique_lock<std::mutex> writeLock(impl().writeMutex);

        bool pass {!record.hasFlags(Record::Flags::Drop)};
        if (pass)
            impl().pipeline.write({}, record);

        if (record.hasFlags(Record::Flags::Flush))
            impl().pipeline.flush();

        writeLock.unlock();

        if (record.hasFlags(Record::Flags::Abort) && !record.hasFlags(Record::Flags::Internal_Queued))
            alog_abort();

        if (record.hasFlags(Record::Flags::Throw) && !record.hasFlags(Record::Flags::Internal_Queued))
            alog_exception(record.getMessage(), record.getMessageLen());

    } else {
        // Add to queue

        std::unique_ptr<std::string> throwText;
        bool abort {record.hasFlags(Record::Flags::Abort) && !record.hasFlags(Record::Flags::Internal_Queued)};

        if (record.hasFlags(Record::Flags::Throw) && !record.hasFlags(Record::Flags::Internal_Queued))
            throwText = std::make_unique<std::string>(record.getMessage(), record.getMessageLen());

        const bool needsFlush {record.hasFlags(Record::Flags::Flush)};
        uint64_t flushTicket {};

        {
            std::lock_guard<std::mutex> lck(impl().queueMutex);
            impl().queue.emplace_back(std::move(record));

            if (needsFlush)
                flushTicket = ++impl().flushTicketsIssued;
        }
        impl().queueCv.notify_one();

        if (needsFlush) {
            std::unique_lock<std::mutex> queueLock(impl().queueMutex);
            impl().flushCv.wait(queueLock, [this, flushTicket]() {
                return isFlushTicketDone(impl().flushTicketsDone, flushTicket);
            });
        }

        if (abort)
            alog_abort();

        if (throwText)
            alog_exception(throwText->c_str(), throwText->size());
    }
}

void Logger::flush()
{
    if (impl().mode == Synchronous) {
        // Sync write
        std::lock_guard<std::mutex> lck(impl().writeMutex);
        impl().pipeline.flush();
    } else {
        // Add to queue & wait
        addRecord(Record::create(Record::Flags::FlushAndDrop));
    }
}

void Logger::setAutoflush(bool value)
{
    impl().autoflush.store(value, std::memory_order_relaxed);
}

void Logger::setMode(Logger::LoggerMode mode)
{
    if (mode == AsynchronousStrictSort) {
        assert(false && "AsynchronousStrictSort not supported yet");
        mode = AsynchronousSort;
    }

    stopThread();
    impl().mode = mode;

    if (impl().mode != Synchronous)
        startThread();
}

Sinks::Pipeline& Logger::pipeline()
{
    return impl().pipeline;
}

const Sinks::Pipeline& Logger::pipeline() const
{
    return impl().pipeline;
}

void Logger::startThread()
{
    assert(impl().mode != Synchronous);
    assert(!impl().threadRunning);
    impl().threadRunning = true;

    impl().exitFlag = false;

    impl().workerThread = std::thread([this](){ threadFunc(); });
}

void Logger::stopThread()
{
    if (!impl().threadRunning) return;
    impl().threadRunning = false;

    {
        std::lock_guard<std::mutex> lck(impl().queueMutex);
        impl().exitFlag = true;
        impl().queueCv.notify_one();
    }

    if (impl().workerThread.joinable())
        impl().workerThread.join();

    impl().queue.clear();
}

void Logger::threadFunc()
{
    std::vector<Record> queue;
    queue.reserve(256);
    bool exitFlag {};
    uint64_t flushTarget {};
    bool flushPending {};

    while (true) {
        {
            std::unique_lock<std::mutex> queueLock(impl().queueMutex);
            impl().queueCv.wait(queueLock, [this]() -> bool { return impl().exitFlag || !impl().queue.empty(); });

            exitFlag = impl().exitFlag;
            std::swap(queue, impl().queue);

            // `addRecord` gets the ticket and queues its record under one `queueMutex` lock,
            // so after this batch is flushed, every ticket issued before this swap is done.
            flushTarget = impl().flushTicketsIssued;
            flushPending = flushTarget != impl().flushTicketsDone;
        }

        if (impl().mode == AsynchronousSort) {
            std::stable_sort(queue.begin(), queue.end(), [](const Record& lhs, const Record& rhs) {
                return lhs.steadyTp < rhs.steadyTp;
            });
        }

        for (const auto& record : queue) {
            const bool pass {!record.hasFlags(Record::Flags::Drop)};
            if (pass) impl().pipeline.write({}, record);
        }

        queue.clear();

        if (flushPending) {
            // We flush once after the whole batch, because a `flush()` record has the earliest timestamp
            // and sorting can move it before the records queued earlier.
            impl().pipeline.flush();
            {
                std::lock_guard<std::mutex> lck(impl().queueMutex);
                impl().flushTicketsDone = flushTarget;
            }
            impl().flushCv.notify_all();
        }

        if (exitFlag) break;
    }
}

} // namespace ALog
