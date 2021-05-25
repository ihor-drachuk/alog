#pragma once
#include <alog/record.h>
#include <alog/filter.h>
#include <alog/sink.h>
#include <alog/tools.h>

namespace ALog {

void alog_abort();
void alog_exception(const char* msg);
void alog_exception(const char* msg, size_t sz);

class Logger
{
public:
    enum LoggerMode {
        Synchronous,
        Asynchronous,
        AsynchronousSort,
        AsynchronousStrictSort
    };

    Logger();
    ~Logger();

    // Thread-safe
    void addRecord(Record&& Record);
    void operator+= (Record&& record) { addRecord(std::move(record)); }
    void flush();
    void setAutoflush(bool value = true);

    // Not thread-safe
    void setMode(LoggerMode mode);
    void setFilter(const IFilterPtr& filter);
    void setSink(const ISinkPtr& sink);

private:
    void startThread();
    void stopThread();
    void threadFunc();
    void prepareEasyMultiSink();
    void addSink(const std::shared_ptr<ISink>& sink, ALog::Severity Severity);

private:
    ALOG_DECLARE_PIMPL
};

} // namespace ALog
