#pragma once
#include <cassert>
#include <vector>
#include <cstdint>
#include <cstring>
#include <utility>
#include <memory>

#if defined(_WIN32) || defined(_WIN64)
#define ALOG_WINDOWS
#endif

#define ALOG_DECLARE_PIMPL  \
    struct impl_t; \
    std::unique_ptr<impl_t> _impl; \
    impl_t& impl() { assert(_impl); return *_impl; } \
    const impl_t& impl() const { assert(_impl); return *_impl; } \
    template<typename... Args> \
    void createImpl(Args&&... args) { assert(!_impl); _impl = std::make_unique<impl_t>(std::forward<Args>(args)...); }

class QString;
class QLatin1String;
class QStringRef;
class QByteArray;
class QPoint;
class QJsonObject;
class QJsonArray;
class QJsonValue;
class QJsonDocument;

#ifdef ALOG_HAS_QT_LIBRARY
#include <QtContainerFwd>
#endif

namespace ALog {

struct Record;

enum Comparison1 { Less, GreaterEqual };

enum no_initialization_tag { no_initialization };

using Buffer = std::vector<uint8_t>;

namespace Internal {

constexpr const char* extractFileNameOnly(const char* str) {
    auto it = str;
    auto lastSlash = str;

    while (*it) {
        if (*it == '/' || *it == '\\') lastSlash = it + 1;
        ++it;
    }

    return lastSlash;
}

bool isSeparatorSymbol(char c);

template<typename Functor>
class Finally {
public:
    Finally(Functor f): m_f(f) {}
    Finally(const Finally&) = delete;
    Finally(Finally&& rhs) noexcept : m_f(rhs.m_f) { rhs.m_moved = true; }
    ~Finally() { if (!m_moved) m_f(); }

    Finally<Functor>& operator=(const Finally<Functor>&) = delete;
    Finally<Functor>& operator=(Finally<Functor>&& rhs) noexcept {
        if (this == &rhs) return *this;

        m_f = rhs.m_f;
        rhs.m_moved = true;

        return *this;
    }

private:
    Functor m_f;
    bool m_moved { false };
};

template<typename Functor>
Finally<Functor> CreateFinally(Functor f) { return Finally<Functor>(f); }

template<size_t sso_limit = 79>
class LongSSO {
public:
    inline LongSSO() {
        m_buf[0] = 0;
    }

    template<size_t N>
    inline LongSSO (const char(&value)[N]) {
        appendString(value, N-1);
    }

    inline LongSSO(Buffer& cache) {
        m_buf[0] = 0;
        m_longBuf = &cache;
    }

    inline LongSSO(LongSSO&& rhs) noexcept {
        *this = std::move(rhs);
    }

    inline LongSSO& operator=(LongSSO&& rhs) noexcept {
        if (this == &rhs) return *this;

        if (m_deleteLongBuf)
            delete m_longBuf;

        m_isShortBuf = rhs.m_isShortBuf;
        m_deleteLongBuf = rhs.m_deleteLongBuf;
        m_longBuf = rhs.m_longBuf;

        if (m_isShortBuf) {
            m_sz = rhs.m_sz;
            memcpy(m_buf, rhs.m_buf, m_sz+1);
        }

        rhs.m_deleteLongBuf = false;
        return *this;
    }

    inline LongSSO(const LongSSO& rhs): LongSSO() {
        *this = rhs;
    }

    inline LongSSO& operator=(const LongSSO& rhs) { // TODO: Remove operator= ?
        if (this == &rhs) return *this;
        clear();
        appendString(rhs.getString(), rhs.getStringLen());
        return *this;
    }

    inline ~LongSSO() {
        if (m_deleteLongBuf)
            delete m_longBuf;
    }

    inline operator bool() const { return getStringLen() != 0; }

    inline uint8_t* allocate_copy(size_t sz, const char* str = nullptr) {
        if (m_isShortBuf) {
            // Short buffer
            size_t newSz = m_sz + sz;
            if (newSz <= sso_limit) {
                uint8_t* target = m_buf + m_sz;
                if (str) memcpy(target, str, sz); // It's 20x faster, than doing memcpy in `appendString`
                m_sz += sz;
                m_buf[m_sz] = 0;
                return target;
            } else {
                auto target = makeLong(sz);
                if (str) memcpy(target, str, sz);
                return target;
            }
        } else {
            // Long buffer
            size_t oldSz = m_longBuf->size() - 1;
            size_t newSz = oldSz + sz;
            m_longBuf->resize(newSz + 1);
            uint8_t* target = m_longBuf->data() + oldSz;
            if (str) memcpy(target, str, sz);
            *(m_longBuf->data() + newSz) = 0;
            return target;
        }
    }

    inline void appendString(const char* str, size_t sz) {
        allocate_copy(sz, str);
    }

    inline void appendStringAL(const char* str) {
        appendString(str, strlen(str));
    }

    template<size_t N>
    inline void appendString(const char(&str)[N]) {
        appendString(str, N-1);
    }

    template<size_t N>
    inline void appendString(const LongSSO<N>& str) {
        appendString(str.getString(), str.getStringLen());
    }

    static_assert (sizeof(uint8_t) == sizeof(char), "Cast validity verification - failed");
    inline const char* getString() const { return reinterpret_cast<const char*>(m_isShortBuf ? m_buf : m_longBuf->data()); }
    inline size_t getStringLen() const { return m_isShortBuf ? m_sz : (m_longBuf->size() - 1); }

    inline size_t getSsoLimit() const { return sso_limit; }
    inline bool isShortString() const { return m_isShortBuf; }

    template<typename... Args>
    inline void appendFmtString(const char* format, Args&&... args) {
        size_t sz = snprintf(nullptr, 0, format, std::forward<Args>(args)...);
        auto target = allocate_copy(sz);
        auto result = snprintf((char*)target, sz+1, format, std::forward<Args>(args)...);
        assert(result == sz);
    }

    inline void clear() {
        m_sz = 0;
        m_buf[0] = 0;
        m_isShortBuf = true;
    }

private:
    inline uint8_t* makeLong(size_t addSz) {
        size_t newSz = m_sz + addSz;

        if (m_longBuf) {
            m_longBuf->resize(newSz+1);
        } else {
            m_longBuf = new Buffer(newSz+1);
            m_deleteLongBuf = true;
        }

        memcpy(m_longBuf->data(), m_buf, m_sz);
        *(m_longBuf->data() + newSz) = 0;

        m_isShortBuf = false;

        return m_longBuf->data() + m_sz;
    }

private:
    uint8_t m_buf[sso_limit+1];
    size_t m_sz { 0 };

    Buffer* m_longBuf { nullptr };

    bool m_isShortBuf { true };
    bool m_deleteLongBuf { false };
};

template<class T>
class Singleton
{
public:
    Singleton() {
        assert(!m_instance);
        m_instance = static_cast<T*>(this);
    }

    ~Singleton() {
        assert(m_instance);
        m_instance = nullptr;
    }

    static T* instance() {
        assert(m_instance);
        return m_instance;
    }

    static bool exists() { return m_instance; }

private:
    static T* m_instance;
};

template<class T>
T* Singleton<T>::m_instance = nullptr;

template<typename T>
class SIOS : public Singleton<SIOS<T>>
{
public:
    template<typename... Args>
    SIOS(Args&&... args) {
        m_object = std::make_shared<T>(std::forward<Args>(args)...);
    }

    std::shared_ptr<T> get() {
        if (!m_ready) return {};
        return m_object;
    }

    T* operator->() { return m_object.get(); }
    const T* operator->() const { return m_object.get(); }
    T& operator*() { return *m_object.get(); }
    const T& operator*() const { return *m_object.get(); }

    bool isReady() const { return m_ready; }
    void markReady() { m_ready = true; }

private:
    bool m_ready { false };
    std::shared_ptr<T> m_object;
};


template<typename T>
class SIOS_Entry
{
public:
    SIOS_Entry() {
        m_created = true;
        renew();
    }

    SIOS_Entry(const SIOS_Entry<T>& rhs) {
        m_object = rhs.m_object;
        m_created = true;
    }

    const std::shared_ptr<T>& get() {
        assert(m_created);
        renew();
        return m_object;
    }

    const std::shared_ptr<T>& fastGet() {
        assert(m_created);
        return m_object;
    }

    bool available() {
        assert(m_created);
        renew();
        return !!m_object;
    }

private:
    void renew() {
        assert(m_created);
        if (m_object) return;
        if (!SIOS<T>::exists()) return;
        if (!SIOS<T>::instance()->isReady()) return;
        m_object = SIOS<T>::instance()->get();
    }

private:
    bool m_created { false };
    std::shared_ptr<T> m_object;
};


namespace ThreadTools {

int currentThreadId();
void setCurrentThreadName(const char* title); // Only literals
const char* currentThreadName();

} // namespace ThreadTools

#pragma pack(push, 1)
class optional_bool
{
public:
    optional_bool() noexcept {}
    optional_bool(bool value) noexcept: m_hasValue(true), m_value(value) {}

    optional_bool(const optional_bool& rhs) noexcept {
        m_hasValue = rhs.m_hasValue;
        m_value = rhs.m_value;
    }

    optional_bool& operator=(const optional_bool& rhs) noexcept {
        if (this != &rhs) {
            m_hasValue = rhs.m_hasValue;
            m_value = rhs.m_value;
        }

        return *this;
    }

    optional_bool& operator=(bool value) noexcept {
        m_hasValue = true;
        m_value = value;
        return *this;
    }

    inline bool has_value() const { return m_hasValue; }

    inline bool value() const {
        assert(m_hasValue);
        return m_value;
    }

    bool value_or(bool value) const noexcept { return m_hasValue ? m_value : value; }

    //operator bool() const noexcept { return m_hasValue; }
    bool operator*() const { return value(); }

private:
    bool m_hasValue { false };
    bool m_value;
};
#pragma pack(pop)

template<typename Interface, typename Class>
class IChain
{
public:
    using ItemPtr = std::shared_ptr<Interface>;
    using ClassPtr = std::shared_ptr<Class>;

public:
    IChain() = default;
    IChain(const std::initializer_list<ItemPtr>& items) { add(items); }
    virtual ~IChain() = default;

    template<typename... Args>
    [[nodiscard]] static ClassPtr create(Args&&... args) { return std::make_shared<Class>(std::forward<Args>(args)...); }
    [[nodiscard]] static ClassPtr create(const std::initializer_list<ItemPtr>& items) { return std::make_shared<Class>(items); }

    void set(const ItemPtr& item) { clear(); add(item); };
    void set(const std::initializer_list<ItemPtr>& items) { clear(); add(items); };
    IChain<Interface, Class>& add(const ItemPtr& item) { m_items.push_back(item); return *this; };
    IChain<Interface, Class>& add(const std::initializer_list<ItemPtr>& items) {
        for (const auto& x : items) m_items.push_back(x);
        return *this;
    }
    virtual void clear() { m_items.clear(); }
    bool empty() const { return m_items.empty(); };

protected:
    using Items = std::vector<ItemPtr>;
    const Items& items() const { return m_items; }

private:
    Items m_items;
};

template<typename T>
struct is_container : public std::false_type {};

template <typename T>
struct has_key : public std::false_type {};

template <typename T>
struct is_qt_container : public std::false_type {}; // TODO: Remove?

#ifdef ALOG_HAS_QT_LIBRARY
template<typename... Args> struct is_container<QCache<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QHash<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QList<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QMap<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QMultiHash<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QMultiMap<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QQueue<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QSet<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QStack<Args...>> : public std::true_type { };
template<typename... Args> struct is_container<QVarLengthArray<Args...>> : public std::true_type { };
template<typename Arg>     struct is_container<QVector<Arg>> : public std::true_type { };
template<>                 struct is_container<QStringList> : public std::true_type { };

template<typename... Args> struct has_key<QCache<Args...>> : public std::true_type { };
template<typename... Args> struct has_key<QHash<Args...>> : public std::true_type { };
template<typename... Args> struct has_key<QMap<Args...>> : public std::true_type { };
template<typename... Args> struct has_key<QMultiHash<Args...>> : public std::true_type { };
template<typename... Args> struct has_key<QMultiMap<Args...>> : public std::true_type { };
#endif // ALOG_HAS_QT_LIBRARY

template <typename T>
struct array_size
{
    static constexpr size_t size = 0;
};

template <typename T, size_t N>
struct array_size<T[N]>
{
    static constexpr size_t size = N;
};

template<size_t N, typename T>
struct container_types { };

template<size_t N, template<typename...> class Container, typename... Ts>
struct container_types<N, Container<Ts...>>
{
    using type = typename std::tuple_element<N, std::tuple<Ts...>>::type;
};

template<typename Key, typename Value, typename Iter>
class qt_iterator_wrapper
{
public:
    using value_type = std::pair<Key, Value>;

    inline qt_iterator_wrapper(Iter iter): m_iter(iter) {
    }

    inline qt_iterator_wrapper& operator++() {
        return ++m_iter;
    }

    inline qt_iterator_wrapper operator++(int) {
        return m_iter++;
    }

    inline bool operator == (const qt_iterator_wrapper& rhs) const {
        return (m_iter == rhs.m_iter);
    }

    inline bool operator != (const qt_iterator_wrapper& rhs) const {
        return !(*this == rhs);
    }

    inline value_type operator*() const {
        return value_type(m_iter.key(), m_iter.value());
    }

private:
    Iter m_iter;
};

template<typename T>
constexpr int combineInt(T value) { return (int)value; }

template<typename T, typename... Ts>
constexpr int combineInt(T value0, Ts... values) { return (int)value0 | combineInt(values...); }

template<typename T>
inline T (max)(T a, T b) { return a > b ? a : b; }

#ifdef ALOG_HAS_QT_LIBRARY
void logJsonData(Record& record, const QString& jsonType, const QString& jsonContent);
#endif // ALOG_HAS_QT_LIBRARY

} // namespace Internal
} // namespace ALog

namespace ALog {
namespace I {
using namespace ::ALog::Internal;
} // namespace I
using Internal::optional_bool;
} // namespace ALog
