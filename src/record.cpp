#include <alog/record.h>

#include <string>
#include <locale>
#include <codecvt>

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
    Record record { uninitialized_tag() };
    record.severity = severity;
    record.line = line;
    record.filenameFull = file;
    record.filenameOnly = fileOnly;
    record.func = func;
    record.threadNum = I::ThreadTools::currentThreadId();
    record.threadTitle = I::ThreadTools::currentThreadName();
    record.module = nullptr;
    record.steadyTp = std::chrono::steady_clock::now();
    record.systemTp = std::chrono::system_clock::now();
    record.flags = defaultFlags;
    record.skipSeparators = 0;

    return record;
}

Record Record::create(Record::Flags flags)
{
    Record record { uninitialized_tag() };
    record.flags = (int)flags;
    return record;
}

static std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t>& utf8_utf16_converter() {
    static thread_local std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> converter;
    return converter;
}

void Record::appendMessage(const wchar_t* msg, size_t len)
{
    std::string dest = utf8_utf16_converter().to_bytes(msg, msg+len);
    appendMessage(dest.data(), dest.size());
}

Record::Record()
{
    severity = Severity::Minimal;
    line = 0;
    filenameFull = nullptr;
    filenameOnly = nullptr;
    func = nullptr;
    threadNum = 0;
    threadTitle = nullptr;
    module = nullptr;
    flags = defaultFlags;
    skipSeparators = 0;
}

} // namespace ALog

ALog::Record&& operator<<(ALog::Record&& record, const ALog::Record::RawData& value)
{
    constexpr size_t bufSz = 64;
    char str[bufSz+1];
    size_t len;

    auto _f = ALog::I::CreateFinally([sepBckp = record.separator, &record](){ record.separator = sepBckp; });
    record.separator.clear();

    if (!value.sz) {
        len = sprintf(str, "{Buffer; Size: 0, Ptr = 0x%p. No data}", value.ptr);
        record.appendMessage(str, len);
        return std::move(record);
    }

    len = sprintf(str, "{Buffer; Size: %zu, Ptr = 0x%p, Data = 0x", value.sz, value.ptr);
    record.appendMessage(str, len);

    size_t len2 = value.sz;
    const uint8_t* ptr = (const uint8_t*)value.ptr;

    while (len2) {
        constexpr size_t printLimit = bufSz / 2;
        size_t limit = len2 < printLimit ? len2 : printLimit;

        for (size_t i = 0; i < limit; i++)
            sprintf(&str[i*2], "%02hhX", *(ptr + i));
        record.appendMessage(str, limit * 2);

        len2 -= limit;
        ptr += limit;
    }

    record.appendMessage("}", 1);

    return std::move(record);
}
