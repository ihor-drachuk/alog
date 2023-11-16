/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/formatters/default.h>

namespace ALog {
namespace Formatters {

static const char* const severitiesMap[] = {
    "Verb",
    "Debug",
    "Info",
    "Warn",
    "Error",
    "Critical"
};

struct Default::impl_t {
    mutable ALog::Buffer cache;
};

Default::Default()
{
    createImpl();
}

Default::~Default()
{
}

Buffer Default::format(const Record& record) const
{
    using namespace std::chrono;

    auto currentTime = (record.steadyTp - record.startTp);
    unsigned long long secs = duration_cast<seconds>(currentTime).count();
    uint16_t msecs = static_cast<uint16_t>(duration_cast<milliseconds>(currentTime).count() - secs * 1000);

    ALog::I::LongSSO<> result(impl().cache);
    result.appendFmtString("[%5llu.%03hu] T#%-2d ", secs, msecs, record.threadNum);

    if (record.threadTitle)
        result.appendFmtString("(%s) ", record.threadTitle);

    result.appendFmtString("[%-8s] ", severitiesMap[record.severity]);

    if (record.module)
        result.appendFmtString("[%-21s] ", record.module);

    result.appendFmtString("[::%s:%d] ", record.func, record.line);

    result.appendString(" ", 1);
    result.appendString(record.getMessage(), record.getMessageLen());

    if (record.hasFlagsAny(Record::Flags::Abort,
                           Record::Flags::Throw))
    {
        result.appendString(" (", 2);
        result.appendStringAL(record.filenameOnly);
        result.appendString(")", 1);
    }

    Buffer resultBuffer(result.getStringLen());
    memcpy(resultBuffer.data(), result.getString(), result.getStringLen());

    return resultBuffer;
}

} // namespace Formatters
} // namespace ALog
