/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/record.h>
#include <cwchar>

#ifdef ALOG_HAS_QT_LIBRARY
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonValue>
#include <QJsonDocument>
#endif // ALOG_HAS_QT_LIBRARY

#ifdef _MSC_VER
    #define snwprintf _snwprintf
#endif // _MSC_VER


#ifdef ALOG_ENABLE_DEF_SEPARATORS
constexpr int defaultFlags_Separators = (int)ALog::Record::Flags::Separators;
#else
constexpr int defaultFlags_Separators = 0;
#endif

#ifdef ALOG_ENABLE_DEF_AUTO_QUOTES
constexpr int defaultFlags_Quotes = (int)ALog::Record::Flags::AutoQuote;
#else
constexpr int defaultFlags_Quotes = 0;
#endif

constexpr int defaultFlags = defaultFlags_Separators + defaultFlags_Quotes;


namespace ALog {

Record Record::create(Severity severity, int line, const char* file, const char* fileOnly, const char* func) {
    Record record {};
    record.severity = severity;
    record.line = line;
    record.filenameFull = file;
    record.filenameOnly = fileOnly;
    record.func = func;
    record.threadNum = I::ThreadTools::currentThreadId();
    record.threadTitle = I::ThreadTools::currentThreadName();
    record.module = nullptr;
    record.steadyTp = std::chrono::steady_clock::now();
    record.flags = defaultFlags;
    record.skipSeparators = 0;

    return record;
}

Record Record::create(Record::Flags flags)
{
    Record record {};
    record.flags = (int)flags;
    return record;
}

void Record::appendMessage(const wchar_t* msg, size_t len, size_t width, char padding)
{
    std::mbstate_t state = std::mbstate_t();
    const wchar_t* srcPtr = msg;
    const size_t srcLen = len;

    size_t dstLen {0}; // Length without \0
#ifdef ALOG_COMPILER_MSVC
    wcsrtombs_s(&dstLen, nullptr, 0, &srcPtr, srcLen, &state);
    dstLen--;
#elif ALOG_COMPILER_GCC || ALOG_COMPILER_CLANG
    dstLen = wcsrtombs(nullptr, &srcPtr, srcLen, &state);
#else
    #error "Unsupported compiler"
#endif

    I::LongSSO tempStr;
    tempStr.allocate_copy(dstLen); // Allocates +1 for \0

#ifdef ALOG_COMPILER_MSVC
    wcsrtombs_s(&dstLen, tempStr.getStringRw(), dstLen+1, &srcPtr, srcLen, &state);
#elif ALOG_COMPILER_GCC || ALOG_COMPILER_CLANG
    wcsrtombs(tempStr.getStringRw(), &srcPtr, srcLen, &state);
#else
    #error "Unsupported compiler"
#endif

    appendMessage(tempStr.getString(), tempStr.getStringLen(), width, padding);
}


} // namespace ALog

ALog::Record&& operator<<(ALog::Record&& record, const ALog::Record::RawData& value)
{
    constexpr size_t strSz = 64;
    constexpr size_t bufSz = strSz + 1;
    char str[bufSz];
    size_t len;

    auto _f = ALog::I::CreateFinally([sepBckp = record.separator, &record](){ record.separator = sepBckp; });
    record.separator.clear();

    if (!value.sz) {
        len = snprintf(str, bufSz, "{Buffer; Size: 0, Ptr = 0x%p. No data}", value.ptr);
        record.appendMessage(str, len);
        return std::move(record);
    }

    len = snprintf(str, bufSz, "{Buffer; Size: %zu, Ptr = 0x%p, Data = 0x", value.sz, value.ptr);
    record.appendMessage(str, len);

    size_t len2 = value.sz;
    const uint8_t* ptr = (const uint8_t*)value.ptr;

    while (len2) {
        constexpr size_t printLimit = strSz / 2;
        size_t limit = len2 < printLimit ? len2 : printLimit;

        for (size_t i = 0; i < limit; i++)
            snprintf(&str[i*2], 3, "%02hhX", *(ptr + i));
        record.appendMessage(str, limit * 2);

        len2 -= limit;
        ptr += limit;
    }

    record.appendMessage("}", 1);

    return std::move(record);
}


#ifdef ALOG_HAS_QT_LIBRARY
ALog::Record&& operator<< (ALog::Record&& record, const QJsonObject& value)
{
    ALog::Internal::logJsonData(record, "Object", QString::fromUtf8(QJsonDocument(value).toJson()));
    return std::move(record);
}

ALog::Record&& operator<< (ALog::Record&& record, const QJsonArray& value)
{
    ALog::Internal::logJsonData(record, "Array", QString::fromUtf8(QJsonDocument(value).toJson()));
    return std::move(record);
}

ALog::Record&& operator<< (ALog::Record&& record, const QJsonValue& value)
{
    if (value.isObject()) {
        record = std::move(record) << value.toObject();

    } else if (value.isArray()) {
        record = std::move(record) << value.toArray();

    } else {
        QJsonObject obj;
        obj[""] = value;
        ALog::Internal::logJsonData(record, "Value", QString::fromUtf8(QJsonDocument(obj).toJson()));

    }

    return std::move(record);
}

ALog::Record&& operator<< (ALog::Record&& record, const QJsonDocument& value)
{
    ALog::Internal::logJsonData(record, "JsonDocument", QString::fromUtf8(value.toJson()));
    return std::move(record);
}
#endif // ALOG_HAS_QT_LIBRARY
