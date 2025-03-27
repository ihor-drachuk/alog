/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/sinks/console.h>

#include <alog/tools_internal.h>
#include <cassert>

namespace ALog {
namespace Sinks {

Console::Console(Console::Stream stream, ColorMode colorMode)
{
    switch (stream) {
        case Stream::StdOut:
            m_handle = stdout;
            break;

        case Stream::StdErr:
            m_handle = stderr;
            break;
    }

    assert(m_handle);

    m_colored = (colorMode == ColorMode::Auto && Internal::enableColoredTerminal(m_handle)) ||
                 colorMode == ColorMode::Force;
}

void Console::write(const Buffer& buffer, const Record& record)
{
    const size_t sz = buffer.size();
    const char* data = reinterpret_cast<const char*>(buffer.data());

    if (m_colored) {
        const std::string& color = Internal::getSeverityColorCode(record.severity);
        const std::string& reset = Internal::getResetColorCode();

        const size_t colorLen = color.size();
        const size_t resetLen = reset.size();

        // Total: color + message + reset + \n
        m_buffer.resize(colorLen + sz + resetLen + 1);

        auto dst = m_buffer.data();
        memcpy(dst, color.data(), colorLen); // NOLINT(bugprone-not-null-terminated-result)

        dst += colorLen;
        memcpy(dst, data, sz);

        dst += sz;
        memcpy(dst, reset.data(), resetLen); // NOLINT(bugprone-not-null-terminated-result)

        dst += resetLen;
        *dst = '\n';

    } else {
        m_buffer.resize(sz + 1);
        memcpy(m_buffer.data(), data, sz);
        m_buffer[sz] = '\n';
    }

    fwrite(m_buffer.data(), 1, m_buffer.size(), m_handle);
    fflush(m_handle);
}

} // namespace Sinks
} // namespace ALog
