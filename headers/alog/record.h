/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <memory>
#include <string>
#include <chrono>
#include <limits>
#include <cstring>
#include <cinttypes>
#include <typeinfo>
#include <utility>
#include <optional>
#include <variant>
#include <alog/tools.h>

#ifdef ALOG_CXX23
#include <expected>
#endif // ALOG_CXX23

#include <jeaiii_to_text.h>

#ifdef ALOG_HAS_QT_LIBRARY
#include <QMetaEnum>
#endif // ALOG_HAS_QT_LIBRARY

namespace ALog {
struct Record;
namespace Internal {
template<typename Iter>
void logArray(Record& record, size_t sz, Iter begin, Iter end);
template<typename SmartPtrType>
void logSmartPtr(Record& record, const SmartPtrType& value, const char* smartPtrName);
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

/* codechecker_false_positive [core.uninitialized.Assign] See https://github.com/llvm/llvm-project/issues/61687 */
struct Record
{
    friend inline void onStringQuote1(Record& record, bool literal);
    friend inline void onStringQuote2(Record& record);
    static constexpr size_t separator_sso_len = 8;

    // Deprecated: "See inline Record(uninitialized_tag)"
    struct uninitialized_tag {};

    enum class Flags {
        Flush = 1,                   // 1
        Drop = 2,                    // 2
        FlushAndDrop = Flush | Drop, // 3 = 1 | 2
        Throw = 4,                   // 4
        ThrowSync = Flush | Throw,   // 5 = 1 | 4
        Abort = 8,                   // 8
        AbortSync = Flush | Abort,   // 9 = 1 | 8

        Separators = 16,             // 16
        NoSeparators = 32,           // 32
        SeparatorsBckp = 64,         // 64
        AutoQuote = 128,             // 128
        NoAutoQuote = 256,           // 256
        QuoteLiterals = 512,         // 512

        Internal_NoSeparators    = 1024,
        Internal_QuoteClose      = 2048,
        Internal_Queued          = 4096,
        Internal_QuoteLiterals   = 8192,
        Internal_RestoreSepBckp = 16384,
    };

    struct RawData {
        [[nodiscard]] static inline RawData create(const void* ptr, size_t sz) { RawData r; r.ptr = ptr; r.sz = sz; return r; };
        const void* ptr;
        size_t sz;
    };

    struct Separator {
        [[nodiscard]] static inline Separator create() { Separator r; return r; };
        [[nodiscard]] static inline Separator create(const char* separator, bool once = false) { Separator r; r.separator.appendStringAL(separator); r.once = once; return r; };
        template<size_t N>
        [[nodiscard]] static inline Separator create(const char(&separator)[N], bool once = false) { Separator r; r.separator.appendString(separator); r.once = once; return r; };
        I::LongSSO<separator_sso_len> separator;
        bool once { false };
    };

    struct SkipSeparator {
        inline SkipSeparator(): count(1) { }
        inline SkipSeparator(int count): count(count) { }
        [[nodiscard]] static inline SkipSeparator create(int count) { return SkipSeparator(count); };
        int count {};
    };

    // -----

    [[nodiscard]] static Record create(Severity severity, int line, const char* file, const char* fileOnly, const char* func);
    [[nodiscard]] static Record create(Flags flags);

    Record() = default;
    [[deprecated("When setting PODs after creation, the optimizer will set them directly. See https://godbolt.org/z/En8Wb3sqe") ]]
    inline Record(uninitialized_tag) { }

    inline void appendMessage(const char* msg, size_t len, size_t width = 0, char padding = ' ') { handleSeparators(len ? *msg : 0); message.appendString(msg, len, width, padding); }
    template<size_t N>
    inline void appendMessage(const char(&msg)[N]) { appendMessage(msg, N-1); }
    inline void appendMessageAL(const char* msg, size_t width = 0, char padding = ' ') { appendMessage(msg, strlen(msg), width, padding); }
    void appendMessage(const wchar_t* msg, size_t len, size_t width = 0, char padding = ' ');

    template<typename T>
    inline void appendInteger(T value, size_t width = 0, char padding = ' ')
    {
        char str[std::numeric_limits<T>::digits10 + 2];
        char* const end = jeaiii::to_text_from_integer(str, value);
        appendMessage(str, end - str, width, padding);
    }

    inline const char* getMessage() const { return message.getString(); }
    inline size_t getMessageLen() const { return message.getStringLen(); }

    [[nodiscard]] inline auto backupFlags() { return I::CreateFinally([this, fl = flags](){ flags = fl; }); }
    [[nodiscard]] inline auto backupSeparators() {
        return I::CreateFinally([this, fl = flags, sep = separator](){
            separator = sep;
            flagsOff(Flags::Separators);
            flagsOn(fl & (int)Flags::Separators);
        });
    }
    inline bool hasFlags(Flags value) const { return (flags & (int)value) == (int)value; }
    inline bool hasFlagsAny(Flags value) const { return (flags & (int)value); }
    inline void flagsOn(Flags value)  { flags |=  (int)value; combineFlags(); }
    inline void flagsOff(Flags value) { flags &= ~(int)value; }

    template<typename... Ts> inline bool hasFlags(Ts... values) const { return (flags & I::combineInt(values...)) == I::combineInt(values...); }
    template<typename... Ts> inline bool hasFlagsAny(Ts... values) const { return (flags & I::combineInt(values...)); }
    template<typename... Ts> inline void flagsOn(Ts... values) { flags |= I::combineInt(values...); combineFlags(); }
    template<typename... Ts> inline void flagsOff(Ts... values) { flags &= ~I::combineInt(values...); }

#ifdef ALOG_ENABLE_DEBUG
    [[nodiscard]] inline auto verifySkipSeparators(int delta = 0) { return I::CreateFinally([this, ss = (I::max)(skipSeparators + delta, 0)](){ assert(ss == skipSeparators); }); }
    #define VERIFY_SKIP_SEPARATORS(record, num) auto _checkSS = record.verifySkipSeparators(num);
#else
    struct Nothing { };
    [[nodiscard]] inline Nothing verifySkipSeparators(int = 0) { return {}; }
    #define VERIFY_SKIP_SEPARATORS(record, num)
#endif

    [[nodiscard]] inline auto updateSkipSeparators(int value) { auto r = verifySkipSeparators(); skipSeparators += value; return r; }
    inline void updateSkipSeparatorsCF(int value) { skipSeparators += value; }

    Record&& seps() { flagsOn(Flags::Separators); return std::move(*this); }
    template<size_t N>
    Record&& seps(const char(&value)[N]) { flagsOn(Flags::Separators); separator.clear(); separator.appendString(value); return std::move(*this); }
    Record&& seps(const char* value) { flagsOn(Flags::Separators); separator.clear(); separator.appendStringAL(value); return std::move(*this); }
    Record&& no_seps() { flagsOn(Flags::NoSeparators); return std::move(*this); }
    Record&& quotes() { flagsOn(Flags::AutoQuote); return std::move(*this); }
    Record&& no_quotes() { flagsOn(Flags::NoAutoQuote); return std::move(*this); }

    Severity severity {};
    int line {};
    const char* filenameFull {};
    const char* filenameOnly {};
    const char* func {};
    int threadNum {};
    const char* threadTitle {}; // Literal ptr
    const char* module {};      // Literal ptr

    std::chrono::time_point<std::chrono::steady_clock> startTp;
    std::chrono::time_point<std::chrono::steady_clock> steadyTp;

    I::LongSSO<> message;
    I::LongSSO<separator_sso_len> separator {" "};

private:
    int flags{};
    int skipSeparators{};

    I::LongSSO<separator_sso_len> separatorBckp;
    int flagsBckp{};

private:
    inline void handleSeparators(char /*nextSymbol*/) {
        if (skipSeparators) {
            skipSeparators--;
            return;
        }

        if (!hasFlags(Flags::Separators) || hasFlags(Flags::Internal_NoSeparators) || !separator || !message) return;

        message.appendString(separator);

        if (hasFlags(Flags::Internal_RestoreSepBckp)) {
            flagsOff(Flags::Internal_RestoreSepBckp);
            separator = separatorBckp;

            flagsOff(Flags::Separators);
            flagsOn((flagsBckp & (int)Flags::Separators) ? (int)Flags::Separators : 0);
        }
    }

    inline void onStringQuote1(bool literal) {
        const bool quoteLiterals = hasFlagsAny(Flags::QuoteLiterals,
                                               Flags::Internal_QuoteLiterals);
        const bool autoQuote = hasFlags(Flags::AutoQuote);

        if (!quoteLiterals && (literal || !autoQuote)) return;
        flagsOn(Flags::Internal_QuoteClose);
        appendMessage("\"", 1);
        skipSeparators++;
    }

    inline void onStringQuote2() {
        if (!hasFlags(Flags::Internal_QuoteClose)) return;
        flagsOff(Flags::Internal_QuoteClose);
        skipSeparators++;
        appendMessage("\"", 1);
    }

    inline void combineFlags() {
        if (hasFlags(ALog::Record::Flags::NoSeparators)) {
            flagsOff(ALog::Record::Flags::NoSeparators, ALog::Record::Flags::Separators);
        }

        if (hasFlags(ALog::Record::Flags::NoAutoQuote)) {
            flagsOff(ALog::Record::Flags::NoAutoQuote, ALog::Record::Flags::AutoQuote);
        }

        if (hasFlags(ALog::Record::Flags::SeparatorsBckp)) {
            flagsOff(ALog::Record::Flags::SeparatorsBckp);
            flagsBckp = flags;
            separatorBckp = separator;
            flagsOn(ALog::Record::Flags::Internal_RestoreSepBckp);
        }
    }
};

inline void onStringQuote1(Record& record, bool literal = false) {
    record.onStringQuote1(literal);
}

inline void onStringQuote2(Record& record) {
    record.onStringQuote2();
}

} // namespace ALog


// --- Forward declarations specially for clang compiler (Mac OS) ---
namespace ALog {
namespace Internal {
template<typename Iter>
void logArray(Record& record, size_t sz, Iter begin, Iter end);
template<typename SmartPtrType>
void logSmartPtr(Record& record, const SmartPtrType& value, const char* smartPtrName);
} // namespace Internal
} // namespace ALog

template <typename T>
inline typename std::enable_if_t<ALog::I::is_container<T>::value && ALog::I::has_key<T>::value && ALog::I::is_qt_container<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value);

template <typename T>
inline typename std::enable_if_t<ALog::I::is_container<T>::value && ALog::I::has_key<T>::value && !ALog::I::is_qt_container<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value);

template <typename T>
inline typename std::enable_if_t<ALog::I::is_container<T>::value && !ALog::I::has_key<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value);

template <typename T>
inline typename std::enable_if_t<std::is_array<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value);

template<typename T1, typename T2>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::pair<T1, T2>& value);

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::shared_ptr<T>& value);

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::unique_ptr<T>& value);

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::weak_ptr<T>& value);

template<typename T>
ALog::Record&& operator<< (ALog::Record&& record, const std::optional<T>& value);

template<typename... Ts>
ALog::Record&& operator<< (ALog::Record&& record, const std::variant<Ts...>& value);

template<typename... Ts>
ALog::Record&& operator<< (ALog::Record&& record, const std::chrono::duration<Ts...>& value);
// -----


inline ALog::Record& operator<< (ALog::Record& record, ALog::Record::Flags flag)
{
    record.flagsOn(flag);
    return record;
}

inline ALog::Record&& operator<< (ALog::Record&& record, ALog::Record::Flags flag)
{
    return (std::move(static_cast<ALog::Record&>(record) << flag));
}

inline ALog::Record& operator-= (ALog::Record& record, ALog::Record::Flags flag)
{
    record.flagsOff(flag);
    return record;
}

inline ALog::Record&& operator-= (ALog::Record&& record, ALog::Record::Flags flag)
{
    return (std::move(static_cast<ALog::Record&>(record) -= flag));
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

inline ALog::Record&& operator<< (ALog::Record&& record, uint8_t value)
{
    record.appendInteger(value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int8_t value)
{
    record.appendInteger(value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, uint16_t value)
{
    record.appendInteger(value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int16_t value)
{
    record.appendInteger(value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, uint32_t value)
{
    record.appendInteger(value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int32_t value)
{
    record.appendInteger(value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, uint64_t value)
{
    record.appendInteger(value);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, int64_t value)
{
    record.appendInteger(value);
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
    VERIFY_SKIP_SEPARATORS(record, -1);
    ALog::onStringQuote1(record, true);
    record.appendMessage(value, strlen(value));
    ALog::onStringQuote2(record);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, const wchar_t* value)
{
    VERIFY_SKIP_SEPARATORS(record, -1);
    ALog::onStringQuote1(record, true);
    record.appendMessage(value, wcslen(value));
    ALog::onStringQuote2(record);
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
    VERIFY_SKIP_SEPARATORS(record, -1);
    ALog::onStringQuote1(record);
    record.appendMessage(value.data(), value.size());
    ALog::onStringQuote2(record);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, const std::wstring& value)
{
    VERIFY_SKIP_SEPARATORS(record, -1);
    ALog::onStringQuote1(record);
    record.appendMessage(value.data(), value.size());
    ALog::onStringQuote2(record);
    return std::move(record);
}

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, const T* value)
{
    [[maybe_unused]] auto _checkSS = record.updateSkipSeparators(4);
    record.appendMessage("(", 1);
    record.appendMessageAL(typeid(T).name());
    record.appendMessage("*)", 2);

    constexpr size_t bufSz = 16+4;
    char str[bufSz];
    size_t len;

    len = snprintf(str, bufSz, "0x%p", value);

    record.appendMessage(str, len);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, const void* value)
{
    constexpr size_t bufSz = 16+4;
    char str[bufSz];
    size_t len;

    len = snprintf(str, bufSz, "0x%p", value);

    [[maybe_unused]] auto _checkSS = record.updateSkipSeparators(1);
    record.appendMessage(str, len);
    return std::move(record);
}

inline ALog::Record&& operator<< (ALog::Record&& record, std::monostate)
{
    record.appendMessage("std::monostate()");
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

#ifdef ALOG_HAS_QT_LIBRARY
template<typename T, typename std::enable_if_t<std::is_enum_v<T> && QtPrivate::IsQEnumHelper<T>::Value>* = nullptr>
inline ALog::Record&& operator<< (ALog::Record&& record, T value) {
    const auto metaEnum = QMetaEnum::fromType<T>();

    record.appendMessageAL(metaEnum.enumName());
    [[maybe_unused]] auto _checkSS = record.updateSkipSeparators(5);
    record.appendMessage("(");
    record = std::move(record) << static_cast<std::underlying_type_t<T>>(value);
    record.appendMessage(", ");

    const auto valueName = metaEnum.valueToKey(static_cast<int>(value));
    if (valueName) {
        record.appendMessageAL(valueName);
    } else {
        record.appendMessage("out-of-range");

        if (record.severity < ALog::Severity::Warning)
            record.severity = ALog::Severity::Warning;
    }

    record.appendMessage(")");
    return std::move(record);
}

template<typename T, typename std::enable_if_t<std::is_enum_v<T> && !QtPrivate::IsQEnumHelper<T>::Value>* = nullptr>
inline ALog::Record&& operator<< (ALog::Record&& record, T value) {
    return (std::move(record) << static_cast<std::underlying_type_t<T>>(value));
}
#else
template<typename T, typename std::enable_if_t<std::is_enum_v<T>>* = nullptr>
inline ALog::Record&& operator<< (ALog::Record&& record, T value) {
    return (std::move(record) << static_cast<std::underlying_type_t<T>>(value));
}
#endif // ALOG_HAS_QT_LIBRARY

ALog::Record&& operator<< (ALog::Record&& record, const ALog::Record::RawData& value);

inline ALog::Record& operator<< (ALog::Record& record, ALog::Record::SkipSeparator ss)
{
    record.updateSkipSeparatorsCF(ss.count);
    return record;
}

inline ALog::Record&& operator<< (ALog::Record&& record, ALog::Record::SkipSeparator ss)
{
    return (std::move(static_cast<ALog::Record&>(record) << ss));
}

inline ALog::Record&& operator<< (ALog::Record&& record, const ALog::Record::Separator& value)
{
    if (value.once)
        record.flagsOn(ALog::Record::Flags::SeparatorsBckp);

    record.separator = value.separator;
    record << ALog::Record::Flags::Separators;
    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QString>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    const auto buffer = value.toUtf8();
    ALog::onStringQuote1(record);
    record.appendMessage(buffer.data(), buffer.size());
    ALog::onStringQuote2(record);
    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QLatin1String>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    ALog::onStringQuote1(record);
    record.appendMessage(value.data(), value.size());
    ALog::onStringQuote2(record);
    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QStringRef>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    const auto buffer = value.toString().toUtf8();
    ALog::onStringQuote1(record);
    record.appendMessage(buffer.data(), buffer.size());
    ALog::onStringQuote2(record);
    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QByteArray>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    return ( std::move(record) << ALog::Record::RawData::create(value.data(), value.size()) );
}

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, QFuture<T> value)
{
    auto _flagsRestorer = record.backupFlags();
    VERIFY_SKIP_SEPARATORS(record, -1);

    if (value.isFinished()) {
        try {
            value.waitForFinished();

        } catch (const std::exception& e) {
            record.appendMessage("QFuture(exception: \"");
            record.updateSkipSeparatorsCF(1);
            record.appendMessageAL(e.what());
            record.updateSkipSeparatorsCF(1);
            record.appendMessage("\")");
            return std::move(record);

        } catch (...) {
            record.appendMessage("QFuture(unknown exception)");
            return std::move(record);
        }

        if (value.isCanceled()) {
            record.appendMessage("QFuture(canceled)");

        } else {
            record.appendMessage("QFuture(finished: ");
            record.updateSkipSeparatorsCF(1);
            record = std::move(record) << value.result();
            record.updateSkipSeparatorsCF(1);
            record.appendMessage(")");
        }

    } else if (value.isStarted()) {
        record.appendMessage("QFuture(running)");

    } else {
        record.appendMessage("QFuture(not started)");
    }

    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QPoint>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    auto _flagsRestorer = record.backupFlags();

    record.appendMessage("QPoint(");
    record << ALog::Record::Flags::Internal_NoSeparators;
    record = std::move(record) << value.x();
    record.appendMessage(", ");
    record = std::move(record) << value.y();
    record.appendMessage(")");

    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QDir>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    auto _flagsRestorer = record.backupFlags();

    record.appendMessage("QDir(");
    record << ALog::Record::Flags::Internal_NoSeparators << ALog::Record::Flags::NoAutoQuote;
    record = std::move(record) << value.path();
    record.appendMessage(")");

    return std::move(record);
}

template<typename T>
inline typename std::enable_if_t<std::is_same<T, QHostAddress>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    auto _flagsRestorer = record.backupFlags();

    record.appendMessage("QHostAddress(");
    record << ALog::Record::Flags::Internal_NoSeparators << ALog::Record::Flags::NoAutoQuote;
    record = std::move(record) << value.toString();
    record.appendMessage(")");

    return std::move(record);
}

#ifdef ALOG_HAS_QT_LIBRARY
template<typename T1, typename T2,
         typename std::enable_if<!std::is_same<QPair<T1, T2>, std::pair<T1, T2>>::value>::type* = nullptr>
inline ALog::Record&& operator<< (ALog::Record&& record, const QPair<T1, T2>& value)
{
    record = std::move(record) << std::pair<T1, T2>{value.first, value.second};
    return std::move(record);
}

ALog::Record&& operator<< (ALog::Record&& record, const QJsonObject& value);
ALog::Record&& operator<< (ALog::Record&& record, const QJsonArray& value);
ALog::Record&& operator<< (ALog::Record&& record, const QJsonValue& value);
ALog::Record&& operator<< (ALog::Record&& record, const QJsonDocument& value);
#endif // ALOG_HAS_QT_LIBRARY

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::shared_ptr<T>& value)
{
    ALog::Internal::logSmartPtr(record, value, "std::shared_ptr");
    return std::move(record);
}

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::unique_ptr<T>& value)
{
    ALog::Internal::logSmartPtr(record, value, "std::unique_ptr");
    return std::move(record);
}

template<typename T>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::weak_ptr<T>& value)
{
    ALog::Internal::logSmartPtr(record, value.lock(), "std::weak_ptr");
    return std::move(record);
}

template<typename T1, typename T2>
inline ALog::Record&& operator<< (ALog::Record&& record, const std::pair<T1, T2>& value)
{
    auto _flagsRestorer = record.backupFlags();
    VERIFY_SKIP_SEPARATORS(record, -1);

    record << ALog::Record::Flags::Internal_QuoteLiterals;

    record.appendMessage("(");
    record = std::move(record) << ALog::Record::SkipSeparator() << value.first;

    record.updateSkipSeparatorsCF(2);
    record.appendMessage(", ");
    record = std::move(record) << value.second;

    record.updateSkipSeparatorsCF(1);
    record.appendMessage(")");

    return std::move(record);
}

namespace ALog {
namespace Internal {

template<typename SmartPtrType>
void logSmartPtr(Record& record, const SmartPtrType& value, const char* smartPtrName)
{
    auto _flagsRestorer = record.backupFlags();
    VERIFY_SKIP_SEPARATORS(record, -1);

    record.appendMessageAL(smartPtrName);
    //record.appendMessage("<");
    //record.appendMessageAL(typeid(SmartPtrType::element_type).name());
    //record.appendMessage(">(");

    record.updateSkipSeparatorsCF(2);
    record.appendMessage("(");

    if (value) {
        record = std::move(record) << *value;
    } else {
        record.appendMessage("nullptr");
    }

    record.updateSkipSeparatorsCF(1);
    record.appendMessage(")");
}

template<typename Iter>
void logArray(Record& record, size_t sz, Iter begin, Iter end)
{
    if (!sz) {
        record.appendMessage("{Container; Size: 0; No data}");
        return;
    }

    auto _flagsRestorer = record.backupFlags();
    VERIFY_SKIP_SEPARATORS(record, -1);

    record << ALog::Record::Flags::Internal_QuoteLiterals;

    auto _separatorRestorer = record.backupSeparators();
    if (!record.hasFlags(ALog::Record::Flags::Separators)) {
        record = std::move(record) << Record::Separator::create(" ");
    }

    auto it = begin;

    record.appendMessage("{Container; Size: ");
    record = std::move(record) << Record::SkipSeparator(2) << sz;
    record.appendMessage("; Data =");

    record = std::move(record) << *it++;

    while (it != end) {
        record = std::move(record) << Record::SkipSeparator(1);
        record.appendMessage(",");
        record = std::move(record) << *it++;
    }

    record = std::move(record) << Record::SkipSeparator(1);
    record.appendMessage("}");
}

} // namespace Internal
} // namespace ALog

template <typename T>
inline typename std::enable_if_t<ALog::I::is_container<T>::value && ALog::I::has_key<T>::value && ALog::I::is_qt_container<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    using Key = typename ALog::I::container_types<0, T>::type;
    using Value = typename ALog::I::container_types<1, T>::type;

    ALog::Internal::logArray(record,
                             value.size(),
                             ALog::I::qt_iterator_wrapper<Key, Value, typename T::const_iterator>(value.cbegin()),
                             ALog::I::qt_iterator_wrapper<Key, Value, typename T::const_iterator>(value.cend()));

    return std::move(record);
}

template <typename T>
inline typename std::enable_if_t<ALog::I::is_container<T>::value && ALog::I::has_key<T>::value && !ALog::I::is_qt_container<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    ALog::Internal::logArray(record, value.size(), value.cbegin(), value.cend());
    return std::move(record);
}

template <typename T>
inline typename std::enable_if_t<ALog::I::is_container<T>::value && !ALog::I::has_key<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    ALog::Internal::logArray(record, value.size(), value.cbegin(), value.cend());
    return std::move(record);
}

template <typename T>
inline typename std::enable_if_t<std::is_array<T>::value, ALog::Record>&& operator<< (ALog::Record&& record, const T& value)
{
    constexpr size_t sz = ALog::I::array_size<T>::size;
    ALog::Internal::logArray(record, sz, value, value + sz);
    return std::move(record);
}

#ifdef ALOG_CXX23
template<typename T1, typename T2>
ALog::Record&& operator<< (ALog::Record&& record, const std::expected<T1, T2>& value)
{
    value ? record.appendMessage("std::expected(") :
            record.appendMessage("std::unexpected(");
    auto _flagsRestorer = record.backupFlags();
    record << ALog::Record::Flags::NoSeparators;

    if (value) {
        record = std::move(record) << value.value();

    } else {
        record.severity = (std::max)(record.severity, ALog::Severity::Warning);
        record = std::move(record) << value.error();
    }

    record.appendMessage(")");

    return std::move(record);
}
#endif // ALOG_CXX23

template<typename T>
ALog::Record&& operator<< (ALog::Record&& record, const std::optional<T>& value)
{
    record.appendMessage("std::optional(");

    auto _flagsRestorer = record.backupFlags();
    record << ALog::Record::Flags::NoSeparators;

    if (value) {
        record = std::move(record) << *value;
    }

    record.appendMessage(")");
    return std::move(record);
}

template<typename... Ts>
ALog::Record&& operator<< (ALog::Record&& record, const std::variant<Ts...>& value)
{
    record.appendMessage("std::variant(");

    auto _flagsRestorer = record.backupFlags();
    record << ALog::Record::Flags::NoSeparators;

    std::visit([&record](const auto& x){
        record = std::move(record) << x;
    }, value);

    record.appendMessage(")");
    return std::move(record);
}

template<typename... Ts>
ALog::Record&& operator<< (ALog::Record&& record, const std::chrono::duration<Ts...>& value)
{
    record.appendMessage("std::chrono::duration(");
    auto _flagsRestorer = record.backupFlags();
    record << ALog::Record::Flags::NoSeparators;

    if (value >= std::chrono::hours(24)) {
        // Days, hours (if != 0)
        const auto days = std::chrono::duration_cast<std::chrono::hours>(value).count() / 24;
        record.appendInteger(days);
        record.appendMessage("d");

        const auto hours = std::chrono::duration_cast<std::chrono::hours>(value).count() - days * 24;
        if (hours > 0) {
            record.appendMessage(" ");
            record.appendInteger(hours);
            record.appendMessage("h");
        }

    } else if (value >= std::chrono::hours(1)) {
        // HH:MM:SS
        const auto hours = std::chrono::duration_cast<std::chrono::hours>(value).count();
        const auto valueMin = value - std::chrono::hours(hours);
        const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(valueMin).count();
        const auto valueSec = valueMin - std::chrono::minutes(minutes);
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(valueSec).count();
        record.appendInteger(hours);
        record.appendMessage("h:");
        record.appendInteger(minutes, 2, '0');
        record.appendMessage("m:");
        record.appendInteger(seconds, 2, '0');
        record.appendMessage("s");

    } else if (value >= std::chrono::minutes(1)) {
        // MM:SS
        const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(value).count();
        const auto valueSec = value - std::chrono::minutes(minutes);
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(valueSec).count();
        record.appendInteger(minutes);
        record.appendMessage("m:");
        record.appendInteger(seconds, 2, '0');
        record.appendMessage("s");

    } else if (value >= std::chrono::seconds(1)) {
        // SS.000
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(value).count();
        const auto valueMs = value - std::chrono::seconds(seconds);
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(valueMs).count();
        record.appendInteger(seconds);
        record.appendMessage(".");
        record.appendInteger(ms, 3, '0');
        record.appendMessage(" sec");

    } else if (value >= std::chrono::milliseconds(10)) {
        // N ms
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(value).count();
        record.appendInteger(ms);
        record.appendMessage(" ms");

    } else {
        // N.000 ms
        const auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(value).count();
        const auto valueUs = value - std::chrono::milliseconds(ms);
        const auto us = std::chrono::duration_cast<std::chrono::microseconds>(valueUs).count();
        record.appendInteger(ms);
        record.appendMessage(".");
        record.appendInteger(us, 3, '0');
        record.appendMessage(" ms");
    }

    record.appendMessage(")");
    return std::move(record);
}


template<typename T> inline ALog::Record& operator<< (ALog::Record& record, T&& value) { return (std::move(record) << std::forward<T&&>(value)); }
template<typename T> inline ALog::Record& operator<< (ALog::Record& record, const T& value) { return (std::move(record) << value); }
