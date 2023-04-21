#pragma once
#include <alog/sink.h>
#include <alog/sinks/console.h>

namespace ALog {
namespace Sinks {

#ifdef ALOG_WINDOWS

#ifdef ALOG_HAS_QT_LIBRARY
class ConsoleQt2 : public ISink
{
public:
    ConsoleQt2() = default;
    ~ConsoleQt2() = default;

    void write(const Buffer& buffer, const Record& record) override;
};
#endif // ALOG_HAS_QT_LIBRARY

#else

using ConsoleQt2 = ALog::Sinks::Console;

#endif // ALOG_WINDOWS

} // namespace Sinks
} // namespace ALog
