/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/sinks/console_utf8.h>

#ifdef ALOG_OS_WINDOWS
#include <alog/tools.h>
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
};

ConsoleUTF8::ConsoleUTF8(ConsoleUTF8::Stream stream)
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
}

ConsoleUTF8::~ConsoleUTF8()
{
    impl().ostream->rdbuf(impl().streamBufBackup);
    delete impl().streamBuf;
}

void ConsoleUTF8::write(const Buffer& buffer, const Record&)
{
    const auto sz = buffer.size();

    impl().buffer.resize(sz + 2);
    memcpy(impl().buffer.data(), buffer.data(), sz);

    impl().buffer[sz] = '\n';
    impl().buffer[sz+1] = 0;

    *impl().ostream << (const char*)impl().buffer.data() << std::flush;
}

} // namespace Sinks
} // namespace ALog

#endif // ALOG_OS_WINDOWS
