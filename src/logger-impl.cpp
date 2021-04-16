#include <alog/logger-impl.h>
#include <alog/exceptions.h>

#include <algorithm>
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
    ISinkPtr sink;
    IFilterPtr filter;

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
    if (!impl().sink) return;

    record.startTp = impl().startTp;

    if (impl().autoflush)
        record.flags |= (int)Record::Flags::Flush;

    if (impl().mode == Synchronous) {
        // Sync write
        std::lock_guard<std::mutex> mx(impl().writeMutex);

        bool pass = ((record.flags & (int)Record::Flags::Drop) == 0) && (impl().filter ? impl().filter->canPass(record).value_or(true) : true);
        if (pass)
            impl().sink->write(record);

        if (record.flags & (int)Record::Flags::Flush)
            impl().sink->flush();

        if ((record.flags & (int)Record::Flags::Abort) && !(record.flags & (int)Record::Flags::Queued))
            alog_abort();

        if ((record.flags & (int)Record::Flags::Throw) && !(record.flags & (int)Record::Flags::Queued))
            alog_exception(record.getMessage(), record.getMessageLen());

    } else {
        // Add to queue

        Record throwMe (Record::uninitialized_tag{});
        bool throwMeValid { false };
        bool abort = (record.flags & (int)Record::Flags::Abort) && !(record.flags & (int)Record::Flags::Queued);

        if ((record.flags & (int)Record::Flags::Throw) && !(record.flags & (int)Record::Flags::Queued)) {
            throwMe = record;
            throwMeValid = true;
        }

        std::unique_lock<std::mutex> lck(impl().queueMutex);
        if (record.flags & (int)Record::Flags::Flush) {
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

        if (throwMeValid)
            alog_exception(throwMe.getMessage(), throwMe.getMessageLen());
    }
}

void Logger::flush()
{
    if (!impl().sink) return;

    if (impl().mode == Synchronous) {
        // Sync write
        std::lock_guard<std::mutex> lck(impl().writeMutex);
        impl().sink->flush();
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

void Logger::setFilter(const IFilterPtr& filter)
{
    impl().filter = filter;
}

void Logger::setSink(const ISinkPtr& sink)
{
    impl().sink = sink;
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
            const auto pass = ((x.flags & (int)Record::Flags::Drop) == 0) && (impl().filter ? impl().filter->canPass(x).value_or(true) : true);
            if (pass) impl().sink->write(x);

            if (x.flags & (int)Record::Flags::Flush) {
                impl().sink->flush();

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
