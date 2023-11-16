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

    Console(Stream stream = Stream::StdOut);

    void write(const Buffer& buffer, const Record& record) override;

private:
    Buffer m_buffer;
    FILE* m_handle { nullptr };
};

} // namespace Sinks
} // namespace ALog
