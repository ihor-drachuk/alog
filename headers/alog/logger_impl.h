/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <alog/record.h>
#include <alog/tools.h>
#include <alog/sinks/pipeline.h>


namespace ALog {

void alog_breakpoint();
void alog_abort();
void alog_exception(const char* msg);
void alog_exception(const char* msg, size_t sz);

class Logger
{
    ALOG_NO_COPY_MOVE(Logger);
public:
    enum LoggerMode {
        Synchronous,
        Asynchronous,
        AsynchronousSort,
        AsynchronousStrictSort
    };

    Logger();
    ~Logger();

    void setupDefaultConfig();

    // Thread-safe
    void addRecord(Record&& Record);
    void operator+= (Record&& record) { addRecord(std::move(record)); }
    void flush();
    void setAutoflush(bool value = true);

    // Not thread-safe
    void setMode(LoggerMode mode);

    ALog::Sinks::Pipeline& pipeline();
    const ALog::Sinks::Pipeline& pipeline() const;

private:
    void startThread();
    void stopThread();
    void threadFunc();

private:
    ALOG_DECLARE_PIMPL
};

} // namespace ALog
