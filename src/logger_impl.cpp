/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/logger_impl.h>

#include <alog/formatters/default.h>
#include <alog/sinks/console.h>

#include <algorithm>
#include <atomic>
#include <cstdint>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>
#include <stdexcept>

#ifdef WIN32
#include <Windows.h>
#define ALOG_STOP_DEBUGGER DebugBreak()
#else
#include <csignal>
#define ALOG_STOP_DEBUGGER raise(SIGTRAP)
#endif

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
    std::thread thread;
    std::condition_variable cv;
    std::vector<Record> queue = [] { std::vector<Record> v; v.reserve(256); return v; }();
    bool exitFlag {};
    bool threadRunning { false };

    // Flush tickets (guarded by queueMutex). Each flush request gets ticket = ++flushRequested.
    // The worker sets flushCompleted = N once every record queued with ticket <= N is written.
    uint64_t flushRequested {};
    uint64_t flushCompleted {};
    std::condition_variable flushCv;

    std::atomic<bool> autoflush { false };

    std::chrono::time_point<std::chrono::steady_clock> startTp = std::chrono::steady_clock::now();
};

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
        std::unique_lock<std::mutex> mx(impl().writeMutex);

        bool pass = !record.hasFlags(Record::Flags::Drop);
        if (pass)
            impl().pipeline.write({}, record);

        if (record.hasFlags(Record::Flags::Flush))
            impl().pipeline.flush();

        mx.unlock(); // Mutex unlocked here!

        if (record.hasFlags(Record::Flags::Abort) && !record.hasFlags(Record::Flags::Internal_Queued))
            alog_abort();

        if (record.hasFlags(Record::Flags::Throw) && !record.hasFlags(Record::Flags::Internal_Queued))
            alog_exception(record.getMessage(), record.getMessageLen());

    } else {
        // Add to queue

        std::unique_ptr<std::string> throwText;
        bool abort = record.hasFlags(Record::Flags::Abort) && !record.hasFlags(Record::Flags::Internal_Queued);

        if (record.hasFlags(Record::Flags::Throw) && !record.hasFlags(Record::Flags::Internal_Queued)) {
            throwText = std::make_unique<std::string>(record.getMessage(), record.getMessageLen());
        }

        const bool needsFlush = record.hasFlags(Record::Flags::Flush);
        uint64_t flushTicket {};

        {
            std::lock_guard<std::mutex> lck(impl().queueMutex);
            impl().queue.emplace_back(std::move(record));

            if (needsFlush)
                flushTicket = ++impl().flushRequested;
        }
        impl().cv.notify_one();

        if (needsFlush) {
            std::unique_lock<std::mutex> lck(impl().queueMutex);
            impl().flushCv.wait(lck, [this, flushTicket](){ return impl().flushCompleted >= flushTicket; });
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

    impl().thread = std::thread([this](){ threadFunc(); });
}

void Logger::stopThread()
{
    if (!impl().threadRunning) return;
    impl().threadRunning = false;

    {
        std::lock_guard<std::mutex> mx(impl().queueMutex);
        impl().exitFlag = true;
        impl().cv.notify_one();
    }

    if (impl().thread.joinable())
        impl().thread.join();

    impl().queue.clear();
}

void Logger::threadFunc()
{
    std::vector<Record> queue;
    queue.reserve(256);
    bool exitFlag {};
    uint64_t flushTarget {};
    bool flushPending {};

    // Dropped records are never written, they only carry Flush (e.g. flush() markers without timestamp).
    // Sort them last, so the flush covers every record of the batch, which includes all records queued before them.
    const auto sortKey = [](const Record& x) {
        return x.hasFlags(Record::Flags::Drop) ? (decltype(x.steadyTp)::max)() : x.steadyTp;
    };
    const auto sortCmp = [&sortKey](const Record& lhs, const Record& rhs) {
        return sortKey(lhs) < sortKey(rhs);
    };

    while (true) {
        {
            std::unique_lock<std::mutex> lck(impl().queueMutex);
            impl().cv.wait(lck, [this]() -> bool { return impl().exitFlag || !impl().queue.empty(); });

            exitFlag = impl().exitFlag;
            std::swap(queue, impl().queue);

            // All records with ticket <= flushTarget are in this batch or in already processed ones
            flushTarget = impl().flushRequested;
            flushPending = flushTarget != impl().flushCompleted;
        }

        if (impl().mode == AsynchronousSort && !std::is_sorted(queue.begin(), queue.end(), sortCmp))
            std::stable_sort(queue.begin(), queue.end(), sortCmp);

        for (const auto& x : queue) {
            const auto pass = !x.hasFlags(Record::Flags::Drop);
            if (pass) impl().pipeline.write({}, x);

            if (x.hasFlags(Record::Flags::Flush))
                impl().pipeline.flush();
        }

        queue.clear();

        if (flushPending) {
            {
                std::lock_guard<std::mutex> lck(impl().queueMutex);
                impl().flushCompleted = flushTarget;
            }
            impl().flushCv.notify_all(); // Several threads may wait for different tickets
        }

        if (exitFlag) break;
    }
}

} // namespace ALog
