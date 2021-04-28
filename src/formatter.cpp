#include <alog/formatter.h>

#include <cstdio>

namespace ALog {

static const char* const severitiesMap[] = {
    "Verb",
    "Debug",
    "Info",
    "Warn",
    "Error",
    "Critical"
};

struct DefaultFormatter::impl_t {
    mutable ALog::Buffer cache;
};

DefaultFormatter::DefaultFormatter()
{
    createImpl();
}

DefaultFormatter::~DefaultFormatter()
{
}

Buffer DefaultFormatter::format(const Record& record) const
{
    using namespace std::chrono;

    auto currentTime = (record.steadyTp - record.startTp);
    unsigned long long secs = duration_cast<seconds>(currentTime).count();
    uint16_t msecs = duration_cast<milliseconds>(currentTime).count() - secs * 1000;

    ALog::LongSSO result(impl().cache);
    result.appendFmtString("[%5llu.%03hu] T#%-2d ", secs, msecs, record.threadNum);

    if (record.threadTitle)
        result.appendFmtString("(%s) ", record.threadTitle);

    result.appendFmtString("[%-8s] ", severitiesMap[record.severity]);

    if (record.module)
        result.appendFmtString("[%-21s] ", record.module);

    result.appendFmtString("[::%s:%d] ", record.func, record.line);

    result.appendString(" ", 1);
    result.appendString(record.getMessage(), record.getMessageLen());

    if ((record.flags & (int)Record::Flags::Abort) ||
        (record.flags & (int)Record::Flags::Throw))
    {
        result.appendString(" (", 2);
        result.appendString(record.filenameOnly);
        result.appendString(")", 1);
    }

    Buffer resultBuffer(result.getStringLen());
    memcpy(resultBuffer.data(), result.getString(), result.getStringLen());

    return resultBuffer;
}

} // namespace ALog
