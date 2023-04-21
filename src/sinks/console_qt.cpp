#include <alog/sinks/console_qt.h>

#ifdef ALOG_WINDOWS
#ifdef ALOG_HAS_QT_LIBRARY

#include <QTextStream>

namespace ALog {
namespace Sinks {

struct ConsoleQt::impl_t
{
    QTextStream* stream { nullptr };
    Buffer buffer;
};

ConsoleQt::ConsoleQt(ConsoleQt::Stream stream)
{
    createImpl();

    switch (stream) {
        case Stream::StdOut:
            impl().stream = new QTextStream(stdout);
            break;

        case Stream::StdErr:
            impl().stream = new QTextStream(stderr);
            break;
    }

    assert(impl().stream);
}

ConsoleQt::~ConsoleQt()
{
    delete impl().stream;
}

void ConsoleQt::write(const Buffer& buffer, const Record&)
{
    assert(buffer.size());

    const auto sz = buffer.size();

    impl().buffer.resize(sz + 2);
    memcpy(impl().buffer.data(), buffer.data(), sz);

    impl().buffer[sz] = '\n';
    impl().buffer[sz+1] = 0;

    *impl().stream << QString((const char*)impl().buffer.data());
    impl().stream->flush();
}

} // namespace Sinks
} // namespace ALog

#endif // ALOG_HAS_QT_LIBRARY
#endif // ALOG_WINDOWS
