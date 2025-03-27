/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

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

    // Define env. variable `QT_CREATOR=1` to make it work in `Auto` mode under Qt Creator
    enum class ColorMode {
        Disable,
        Auto,
        Force
    };

    ConsoleUTF8(Stream stream, ColorMode colorMode = ColorMode::Auto);
    ~ConsoleUTF8() override;

    void write(const Buffer& buffer, const Record& record) override;

private:
    ALOG_DECLARE_PIMPL
};

#else

using ConsoleUTF8 = Console;

#endif // ALOG_OS_WINDOWS

} // namespace Sinks
} // namespace ALog
