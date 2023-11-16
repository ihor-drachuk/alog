/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/sinks/console.h>

namespace ALog {
namespace Sinks {

Console::Console(Console::Stream stream)
{
    switch (stream) {
        case Stream::StdOut:
            m_handle = stdout;
            break;

        case Stream::StdErr:
            m_handle = stderr;
            break;
    }
}

void Console::write(const Buffer& buffer, const Record&)
{
    const auto sz = buffer.size();

    m_buffer.resize(sz + 1);
    memcpy(m_buffer.data(), buffer.data(), sz);

    *(char*)(m_buffer.data() + sz) = '\n';

    fwrite((const char*)m_buffer.data(), 1, m_buffer.size(), m_handle);
    fflush(m_handle); // Flush always
}

} // namespace Sinks
} // namespace ALog
