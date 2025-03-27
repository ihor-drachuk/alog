/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <alog/sink.h>

namespace ALog {
namespace Sinks {

class Console : public ISink
{
public:
    enum class Stream {StdOut, StdErr};

    // Define env. variable `QT_CREATOR=1` to make it work in `Auto` mode under Qt Creator
    enum class ColorMode {
        Disable,
        Auto,
        Force
    };

    Console(Stream stream = Stream::StdOut, ColorMode colorMode = ColorMode::Auto);

    void write(const Buffer& buffer, const Record& record) override;

private:
    Buffer m_buffer;
    FILE* m_handle { nullptr };
    bool m_colored {};
};

} // namespace Sinks
} // namespace ALog
