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

Buffer DefaultFormatter::format(const Record& record) const
{
    using namespace std::chrono;

    // [xxxxx.xxx] __1 (Thread-name) Critical [Module]  Text message
    static const char* const format =             "[%5llu.%03hu] T#%-2d [%-8s]  %s";
    static const char* const formatThread =       "[%5llu.%03hu] T#%-2d (%s) [%-8s]  %s";
    static const char* const formatModule =       "[%5llu.%03hu] T#%-2d [%-8s] [%s]  %s";
    static const char* const formatThreadModule = "[%5llu.%03hu] T#%-2d (%s) [%-8s] [%s]  %s";

    static const char* const addFormatExcpAbrt =             "; Function: '%s'. File: '%s'. Line: %d";

    auto currentTime = (record.steadyTp - record.startTp);
    unsigned long long secs = duration_cast<seconds>(currentTime).count();
    uint16_t msecs = duration_cast<milliseconds>(currentTime).count() - secs * 1000;

    Buffer buf;
    size_t bufSz;

    const char* msgPtr = record.getMessageLen() ? record.getMessage() : "";

    if (record.module) {
        if (record.threadTitle) {
            bufSz = snprintf(nullptr, 0, formatThreadModule, secs, msecs, record.threadNum, record.threadTitle, severitiesMap[record.severity], record.module, msgPtr);
            bufSz++;
            buf.resize(bufSz * sizeof(char));
            sprintf((char*)buf.data(), formatThreadModule, secs, msecs, record.threadNum, record.threadTitle, severitiesMap[record.severity], record.module, msgPtr);
        } else {
            bufSz = snprintf(nullptr, 0, formatModule, secs, msecs, record.threadNum, severitiesMap[record.severity], record.module, msgPtr);
            bufSz++;
            buf.resize(bufSz * sizeof(char));
            sprintf((char*)buf.data(), formatModule, secs, msecs, record.threadNum, severitiesMap[record.severity], record.module, msgPtr);
        }
    } else {
        if (record.threadTitle) {
            bufSz = snprintf(nullptr, 0, formatThread, secs, msecs, record.threadNum, record.threadTitle, severitiesMap[record.severity], msgPtr);
            bufSz++;
            buf.resize(bufSz * sizeof(char));
            sprintf((char*)buf.data(), formatThread, secs, msecs, record.threadNum, record.threadTitle, severitiesMap[record.severity], msgPtr);
        } else {
            bufSz = snprintf(nullptr, 0, format, secs, msecs, record.threadNum, severitiesMap[record.severity], msgPtr);
            bufSz++;
            buf.resize(bufSz * sizeof(char));
            sprintf((char*)buf.data(), format, secs, msecs, record.threadNum, severitiesMap[record.severity], msgPtr);
        }
    }

    if ((record.flags & (int)Record::Flags::Abort) ||
        (record.flags & (int)Record::Flags::Throw))
    {
        size_t textSz2 = snprintf(nullptr, 0, addFormatExcpAbrt, record.func, record.file, record.line);
        buf.resize(buf.size() + textSz2 * sizeof(char));
        sprintf((char*)buf.data() + bufSz - 1, addFormatExcpAbrt, record.func, record.file, record.line);
    }

    buf.resize(buf.size()-1);

    return buf;
}

} // namespace ALog
