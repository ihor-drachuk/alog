#include <alog/logger_impl.h>
#include <alog/exceptions.h>

#include <algorithm>
#include <memory>
#include <mutex>
#include <thread>
#include <condition_variable>

namespace ALog {

void alog_abort()
{
    std::abort();
}

void alog_exception(const char* msg)
{
    alog_exception(msg, strlen(msg));
}

void alog_exception(const char* msg, size_t)
{
    throw ALog::runtime_error_wide(msg);
}


struct Logger::impl_t
{
    ALog::Sinks::Pipeline pipeline;

    LoggerMode mode;
    std::mutex writeMutex;
    std::mutex queueMutex;
    std::thread thread;
    std::condition_variable cv;
    std::vector<Record> queue;
    volatile bool exitFlag;
    bool threadRunning { false };

    bool flushRequested { false };
    std::condition_variable flushCv;

    bool autoflush { false };

    std::chrono::time_point<std::chrono::steady_clock> startTp = std::chrono::steady_clock::now();
};

Logger::Logger()
{
    createImpl();
    setMode(AsynchronousSort);
}

Logger::~Logger()
{
    flush();
    stopThread();
}

void Logger::addRecord(Record&& record)
{
    record.startTp = impl().startTp;

    if (impl().autoflush)
        record.flagsOn(Record::Flags::Flush);

    if (impl().mode == Synchronous) {
        // Sync write
        std::lock_guard<std::mutex> mx(impl().writeMutex);

        bool pass = !record.hasFlags(Record::Flags::Drop);
        if (pass)
            impl().pipeline.write({}, record);

        if (record.hasFlags(Record::Flags::Flush))
            impl().pipeline.flush();

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

        std::unique_lock<std::mutex> lck(impl().queueMutex);
        if (record.hasFlags(Record::Flags::Flush)) {
            if (!record.steadyTp.time_since_epoch().count())
                record.steadyTp = decltype(record.steadyTp)::max();

            impl().flushRequested = true;
            impl().queue.emplace_back(std::move(record));
            impl().cv.notify_one();
            impl().flushCv.wait(lck, [this](){ return !impl().flushRequested; });
        } else {
            impl().queue.emplace_back(std::move(record));
            impl().cv.notify_one();
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
    impl().autoflush = value;
}

void Logger::setMode(Logger::LoggerMode mode)
{
    if (mode == AsynchronousStrictSort) {
        assert(!"AsynchronousStrictSort not supported yet");
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
    bool exitFlag;

    while (true) {
        {
            std::unique_lock<std::mutex> lck(impl().queueMutex);
            impl().cv.wait(lck, [this]() -> bool { return impl().exitFlag || !impl().queue.empty(); });

            exitFlag = impl().exitFlag;
            std::swap(queue, impl().queue);
        }

        if (impl().mode == AsynchronousSort) {
            std::stable_sort(queue.begin(), queue.end(), [](const Record& lhs, const Record& rhs) {
                return lhs.steadyTp < rhs.steadyTp;
            });
        }

        for (const auto& x : queue) {
            const auto pass = !x.hasFlags(Record::Flags::Drop);
            if (pass) impl().pipeline.write({}, x);

            if (x.hasFlags(Record::Flags::Flush)) {
                impl().pipeline.flush();

                {
                    std::unique_lock<std::mutex> lck(impl().queueMutex);
                    impl().flushRequested = false;
                    impl().flushCv.notify_one();
                }
            }
        }

        queue.clear();

        if (exitFlag) break;
    }
}

} // namespace ALog
