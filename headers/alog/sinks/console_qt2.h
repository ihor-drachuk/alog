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
    // Define env. variable `QT_CREATOR=1` to make it work in `Auto` mode under Qt Creator
    enum class ColorMode {
        Disable,
        Auto,
        Force
    };

    ConsoleQt2(ColorMode colorMode = ColorMode::Auto);
    ~ConsoleQt2() override;

    void write(const Buffer& buffer, const Record& record) override;

private:
    ALOG_DECLARE_PIMPL
};
#endif // ALOG_HAS_QT_LIBRARY

#else

using ConsoleQt2 = ALog::Sinks::Console;

#endif // ALOG_OS_WINDOWS

} // namespace Sinks
} // namespace ALog
