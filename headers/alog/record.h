#pragma once
#include <string>
#include <sstream>
#include <chrono>
#include <optional>
#include <limits>
#include <cstring>
#include <cinttypes>
#include <alog/tools.h>

namespace ALog {
struct Record;
namespace Internal {
template<typename Iter>
void logArray(Record& record, size_t sz, Iter begin, Iter end);
} // namespace Internal
} // namespace ALog

namespace ALog {

enum Severity {
    Minimal,
    Verbose = Minimal,
    Debug,
    Info,
    Warning,
    Error,
    Fatal,
    Maximal = Fatal
};

struct Record
{
    struct uninitialized_tag {};

    enum class Flags {
        Flush = 1,                   // 1
        Drop = 2,                    // 2
        FlushAndDrop = Flush | Drop, //    1 | 2
        Throw = 4,                   // 4
        ThrowSync = Flush | Throw,   //    1 | 4
        Abort = 8,                   // 8
        AbortSync = Flush | Abort,   //    1 | 8
        NeedSeparator = 16,          // 16
        Queued = 32,                 // 32
    };

    struct RawData {
        [[nodiscard]] static RawData create(const void* ptr, size_t sz);
        const void* ptr;
        size_t sz;
    };

    // -----

    static constexpr size_t sso_str_len = 79;
    [[nodiscard]] static Record create(Severity severity, int line, const char* file, const char* fileOnly, const char* func);
    [[nodiscard]] static Record create(Flags flags);

    Record();
    Record(uninitialized_tag) { }

    void appendMessage(const char* msg, size_t len);
    void appendMessage(const wchar_t* msg, size_t len);

    inline const char* getMessage() const { return str.getString(); }
    inline size_t getMessageLen() const { return str.getStringLen(); }

    Severity severity;
    int line;
    const char* filenameFull;
    const char* filenameOnly;
    const char* func;
    int threadNum;
    const char* threadTitle; // Literal ptr
    const char* module;      // Literal ptr
    int flags;
    std::chrono::time_point<std::chrono::steady_clock> startTp;
    std::chrono::time_point<std::chrono::steady_clock> steadyTp;
    std::chrono::time_point<std::chrono::system_clock> systemTp;

    LongSSO<sso_str_len> str;

private:
    inline void handleSeparators() {
        if (flags & (int)Flags::NeedSeparator) {
            flags ^= (int)Flags::NeedSeparator;
            appendMessage(". ", 2);
        }
    }
};

} // namespace ALog

inline ALog::Record& operator<< (ALog::Record& record, ALog::Record::Flags flag)
{
    record.flags |= (int)flag;
    return record;
}

inline ALog::Record&& operator<< (ALog::Record&& record, ALog::Record::Flags flag)
{
    return (std::move(static_cast<ALog::Record&>(record) << flag));
}

inline ALog::Record&& operator<< (ALog::Record&& record, bool value)
{
    if (value) {
        record.appendMessage("true", 4);
    } else {
        record.appendMessage("false", 5);
    }

    return std::move(record);
}

namespace ALog {
namespace Internal {
#if defined(ALOG_MACOSX) || defined(ALOG_LINUX)
    inline void itoa(int value, char* dst, int radix) {
        assert(radix == 10);
        sprintf(dst, "%d", value);
    }

    inline void ltoa(long value, char* dst, int radix) {
        assert(radix == 10);
        sprintf(dst, "%ld", value);
    }

    inline void ultoa(unsigned long value, char* dst, int radix) {
        assert(radix == 10);
        sprintf(dst, "%lu", value);
    }
#endif

template<typename T>
inline void addInteger(ALog::Record& record, T value)
{
    constexpr size_t bufSz = std::numeric_limits<T>::digits + 2;
    char str[bufSz];
    size_t len;

    if constexpr (sizeof(T) < sizeof(int)) {
        itoa(value, str, 10);
    } else if constexpr (sizeof(T) == sizeof(int) && std::numeric_limits<T>::is_signed) {
        itoa(value, str, 10);
    } else if constexpr (sizeof(T) <= sizeof(long) && std::numeric_limits<T>::is_signed) {
        ltoa(value, str, 10);
    } else if constexpr (sizeof(T) <= sizeof(long) && !std::numeric_limits<T>::is_signed) {
#ifdef __MINGW32__
        sprintf(str, "%lu", value);
#else
        ultoa(value, str, 10);
#endif
    }

    len = strlen(str);

    record.appendMessage(str, len);
}
} // namespace Internal
} // namespace ALog


inline ALog::Record&& operator<< (ALog::Record&& record, uint8_t value)
{
    ALog::Internal::addInteger(record, value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int8_t value)
{
    ALog::Internal::addInteger(record, value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, uint16_t value)
{
    ALog::Internal::addInteger(record, value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int16_t value)
{
    ALog::Internal::addInteger(record, value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, uint32_t value)
{
    ALog::Internal::addInteger(record, value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int32_t value)
{
    ALog::Internal::addInteger(record, value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, uint64_t value)
{
    constexpr size_t bufSz = std::numeric_limits<decltype(value)>::digits + 2;
    char str[bufSz];
    size_t len;

    len = sprintf(str, "%" PRIu64, value);

    record.appendMessage(str, len);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int64_t value)
{
    constexpr size_t bufSz = std::numeric_limits<decltype(value)>::digits + 2;
    char str[bufSz];
    size_t len;

    len = sprintf(str, "%" PRId64, value);

    record.appendMessage(str, len);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, float value)
{
    constexpr size_t bufSz = 1024;
    char str[bufSz];
    size_t len;

    len = snprintf(str, bufSz, "%f", (double)value);

    record.appendMessage(str, len);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, double value)
{
    constexpr size_t bufSz = 1024;
    char str[bufSz];
    size_t len;

    len = snprintf(str, bufSz, "%f", value);

    record.appendMessage(str, len);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, long double value)
{
    constexpr size_t bufSz = 1024;
    char str[bufSz];
    size_t len;

    len = snprintf(str, bufSz, "%Lf", value);

    record.appendMessage(str, len);
    return std::move(record);
}

#if 0
template<size_t N>
inline ALog::Record&& operator<< (ALog::Record&& record, const char(&value)[N])
{
    if (value[N-1]) {
        ALog::Internal::logArray(record, N, value, value + N);
    } else {
        record.appendMessage(value, N-1);
    }

    return std::move(record);
}

template<size_t N>
inline ALog::Record&& operator<< (ALog::Record&& record, const wchar_t(&value)[N])
{
    if (value[N-1]) {
        ALog::Internal::logArray(record, N, value, value + N);
    } else {
        record.appendMessage(value, N-1);
    }

    return std::move(record);
}

template<typename T, typename std::enable_if_t<std::is_same<T, char*>::value>* = nullptr>
inline ALog::Record&& operator<< (ALog::Record&& record, T value)
{
    record.appendMessage(value, strlen(value));
    return std::move(record);
}

template<typename T, typename std::enable_if_t<std::is_same<T, wchar_t*>::value>* = nullptr>
inline ALog::Record&& operator<< (ALog::Record&& record, T value)
{
    record.appendMessage(value, wcslen(value));
    return std::move(record);
}
#else
inline ALog::Record&& operator<< (ALog::Record&& record, const char* value)
{
    record.appendMessage(value, strlen(value));
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, const wchar_t* value)
{
    record.appendMessage(value, wcslen(value));
    return std::move(record);
}
#endif

inline ALog::Record&& operator<< (ALog::Record&& record, char value)
{
    record.appendMessage(&value, 1);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, wchar_t value)
{
    record.appendMessage(&value, 1);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, const std::string& value)
{
    record.appendMessage(value.data(), value.size());
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, const std::wstring& value)
{
    record.appendMessage(value.data(), value.size());
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, const void* value)
{
    constexpr size_t bufSz = 16+4;
    char str[bufSz];
    size_t len;

    len = sprintf(str, "0x%p", value);

    record.appendMessage(str, len);
    return std::move(record);
}

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && !std::is_signed<T>::value && sizeof(T) == 1>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (uint8_t)value); }

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && !std::is_signed<T>::value && sizeof(T) == 2>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (uint16_t)value); }

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && !std::is_signed<T>::value && sizeof(T) == 4>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (uint32_t)value); }

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && !std::is_signed<T>::value && sizeof(T) == 8>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (uint64_t)value); }

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value && sizeof(T) == 1>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (int8_t)value); }

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value && sizeof(T) == 2>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (int16_t)value); }

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value && sizeof(T) == 4>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (int32_t)value); }

template<typename T, typename std::enable_if_t<std::is_integral<T>::value && std::is_signed<T>::value && sizeof(T) == 8>* = nullptr >
inline ALog::Record&& operator<< (ALog::Record&& record, T value) { return (std::move(record) << (int64_t)value); }

ALog::Record&& operator<< (ALog::Record&& record, const ALog::Record::RawData& value);

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QString>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    const auto buffer = value.toUtf8();
    record.appendMessage(buffer.data(), buffer.size());
    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QLatin1String>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    record.appendMessage(value.data(), value.size());
    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QStringRef>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    const auto buffer = value.toString().toUtf8();
    record.appendMessage(buffer.data(), buffer.size());
    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QByteArray>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    return ( std::move(record) << ALog::Record::RawData::create(value.data(), value.size()) );
}

template<typename T1, typename T2>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::pair<T1, T2>& value)
{
    std::move(record) << "(";
    std::move(record) << value.first;
    std::move(record) << ", ";
    std::move(record) << value.second;
    std::move(record) << ")";
    return std::move(record);
}

namespace ALog {
namespace Internal {

template<typename Iter>
void logArray(Record& record, size_t sz, Iter begin, Iter end)
{
    if (!sz) {
        std::move(record) << "{Container; Size: 0; No data}";
        return;
    }

    std::move(record) << "{Container; Size: " << sz << "; Data = ";

    auto it = begin;
    std::move(record) << *it++;

    while (it != end) {
        std::move(record) << ", " << *it++;
    }

    std::move(record) << "}";
}

} // namespace Internal
} // namespace ALog

template <typename T>
inline typename std::enable_if_t<ALog::is_container<T>::value && ALog::has_key<T>::value && ALog::is_qt_container<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    using Key = typename ALog::container_types<0, T>::type;
    using Value = typename ALog::container_types<1, T>::type;

    ALog::Internal::logArray(record,
                             value.size(),
                             ALog::qt_iterator_wrapper<Key, Value, typename T::const_iterator>(value.cbegin()),
                             ALog::qt_iterator_wrapper<Key, Value, typename T::const_iterator>(value.cend()));

    return std::move(record);
}

template <typename T>
inline typename std::enable_if_t<ALog::is_container<T>::value && ALog::has_key<T>::value && !ALog::is_qt_container<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    ALog::Internal::logArray(record, value.size(), value.cbegin(), value.cend());
    return std::move(record);
}

template <typename T>
inline typename std::enable_if_t<ALog::is_container<T>::value && !ALog::has_key<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    ALog::Internal::logArray(record, value.size(), value.cbegin(), value.cend());
    return std::move(record);
}

template <typename T>
inline typename std::enable_if_t<std::is_array<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    constexpr size_t sz = ALog::array_size<T>::size;
    ALog::Internal::logArray(record, sz, value, value + sz);
    return std::move(record);
}
