#pragma once
#include <alog/sink.h>
#include <alog/sinks/console.h>

namespace ALog {
namespace Sinks {

#ifdef ALOG_OS_WINDOWS

class ConsoleUTF8 : public ISink
{
public:
    enum class Stream {StdOut, StdErr};

    ConsoleUTF8(Stream stream);
    ~ConsoleUTF8();

    void write(const Buffer& buffer, const Record& record);

private:
    ALOG_DECLARE_PIMPL
};

#else

using ConsoleUTF8 = Console;

#endif // ALOG_OS_WINDOWS

} // namespace Sinks
} // namespace ALog
