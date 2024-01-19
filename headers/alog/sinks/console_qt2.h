/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <alog/sink.h>
#include <alog/sinks/console.h>

namespace ALog {
namespace Sinks {

#ifdef ALOG_OS_WINDOWS

#ifdef ALOG_HAS_QT_LIBRARY
class ConsoleQt2 : public ISink
{
public:
    ConsoleQt2() = default;
    ~ConsoleQt2() override = default;

    void write(const Buffer& buffer, const Record& record) override;
};
#endif // ALOG_HAS_QT_LIBRARY

#else

using ConsoleQt2 = ALog::Sinks::Console;

#endif // ALOG_OS_WINDOWS

} // namespace Sinks
} // namespace ALog
