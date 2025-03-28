/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/formatters/default.h>

namespace ALog {
namespace Formatters {

namespace {

static const char* const severitiesMap[] = {
    "Verb",
    "Debug",
    "Info",
    "Warn",
    "Error",
    "Crit!"
};

template<typename T, typename Timepoint>
unsigned long long unsigned_duration_cast(const Timepoint& tp)
{
    return static_cast<unsigned long long>(std::chrono::duration_cast<T>(tp).count());
}

} // namespace

struct Default::impl_t
{
    mutable ALog::Buffer cache;
    Internal::Flags<Flag> flags;
};

Default::Default(Internal::Flags<Flag> flags)
{
    createImpl();
    impl().flags = flags;
}

Default::~Default() = default;

Buffer Default::format(const Record& record) const
{
    ALog::I::LongSSO<> result(impl().cache);

    if (impl().flags & Flag::DateTime) {
        std::time_t timeT = std::chrono::system_clock::to_time_t(record.systemTp);
        std::tm tmTime;
#ifdef _WIN32
        localtime_s(&tmTime, &timeT);
#else
        localtime_r(&timeT, &tmTime);
#endif // _WIN32
        result.appendFmtString("[%02d%02d%02d_%02d%02d%02d] ",
                               tmTime.tm_year % 100, tmTime.tm_mon + 1, tmTime.tm_mday,
                               tmTime.tm_hour, tmTime.tm_min, tmTime.tm_sec);
    }

    if (impl().flags & Flag::LocalTimestamp) {
        const auto currentTime = (record.steadyTp - record.startTp);
        const unsigned long long secs = unsigned_duration_cast<std::chrono::seconds>(currentTime);
        const uint16_t msecs = static_cast<uint16_t>(unsigned_duration_cast<std::chrono::milliseconds>(currentTime) - secs * 1000); // NOLINT(hicpp-use-auto)
        result.appendFmtString("[%3llu.%03hu] ", secs, msecs);
    }

    result.appendFmtString("T#%-2d ", record.threadNum);

    if (record.threadTitle)
        result.appendFmtString("(%s) ", record.threadTitle);

    result.appendFmtString("[%-5s] ", severitiesMap[record.severity]);

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
