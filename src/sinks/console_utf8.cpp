/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/sinks/console_utf8.h>

#ifdef ALOG_OS_WINDOWS
#include <alog/tools.h>
#include <alog/tools_internal.h>
#include <Windows.h>
#include <iostream>
#include <sstream>

namespace ALog {
namespace Sinks {

class MBuf: public std::stringbuf {
public:
    MBuf(FILE* stream): m_stream(stream) { }

    int sync() override {
        fputs(str().c_str(), m_stream);
        str("");
        return 0;
    }

private:
    FILE* m_stream;
};

struct ConsoleUTF8::impl_t
{
    Buffer buffer;

    decltype (std::cout.rdbuf()) streamBufBackup { nullptr };
    MBuf* streamBuf { nullptr };

    std::ostream* ostream { nullptr };
    bool colored {};
};

ConsoleUTF8::ConsoleUTF8(ConsoleUTF8::Stream stream, ColorMode colorMode)
{
    createImpl();

    FILE* selectedStream = nullptr;

    switch (stream) {
        case Stream::StdOut:
            selectedStream = stdout;
            impl().ostream = &std::cout;
            break;

        case Stream::StdErr:
            selectedStream = stderr;
            impl().ostream = &std::cerr;
            break;
    }

    assert(selectedStream);
    assert(impl().ostream);

    SetConsoleOutputCP(CP_UTF8);
    setvbuf(selectedStream, nullptr, _IONBF, 0);
    impl().streamBufBackup = impl().ostream->rdbuf();

    impl().streamBuf = new MBuf(selectedStream);
    impl().ostream->rdbuf(impl().streamBuf);

    impl().colored = (colorMode == ColorMode::Auto && Internal::enableColoredTerminal(selectedStream)) ||
                      colorMode == ColorMode::Force;
}

ConsoleUTF8::~ConsoleUTF8()
{
    impl().ostream->rdbuf(impl().streamBufBackup);
    delete impl().streamBuf;
}

void ConsoleUTF8::write(const Buffer& buffer, const Record& record)
{
    const auto sz = buffer.size();

    if (impl().colored) {
        const std::string& color = Internal::getSeverityColorCode(record.severity);
        const std::string& reset = Internal::getResetColorCode();

        const size_t colorLen = color.size();
        const size_t resetLen = reset.size();

        // color + message + reset + \n
        impl().buffer.resize(colorLen + sz + resetLen + 1);

        auto dst = impl().buffer.data();
        memcpy(dst, color.data(), colorLen);

        dst += colorLen;
        memcpy(dst, buffer.data(), sz);

        dst += sz;
        memcpy(dst, reset.data(), resetLen);

        dst += resetLen;
        *dst = '\n';

    } else {
        impl().buffer.resize(sz + 1);
        memcpy(impl().buffer.data(), buffer.data(), sz);
        impl().buffer[sz] = '\n';
    }

    impl().ostream->write(reinterpret_cast<const char*>(impl().buffer.data()),
                          static_cast<std::streamsize>(impl().buffer.size()));
    impl().ostream->flush();
}

} // namespace Sinks
} // namespace ALog

#endif // ALOG_OS_WINDOWS
