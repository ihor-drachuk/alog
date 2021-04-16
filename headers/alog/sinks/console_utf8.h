#pragma once
#include <alog/sink.h>

namespace ALog {

#ifdef ALOG_WINDOWS

class ConsoleUTF8 : public ISinkWithFmtCnv
{
public:
    enum class Stream {
        StdOut,
        StdErr
    };

    ConsoleUTF8(Stream stream);
    ~ConsoleUTF8();

protected:
    void writeBuffer(const Buffer& buffer) override;

private:
    ALOG_DECLARE_PIMPL
};

#else

using ConsoleUTF8 = SinkStdStream;

#endif // ALOG_WINDOWS

} // namespace ALog
