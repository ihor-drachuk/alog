#pragma once
#include <cassert>
#include <vector>
#include <cstdint>
#include <utility>
#include <memory>
#include <alog/tools_fwd.h>

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
template<typename> class QList;
template<typename> class QLinkedList;
template<typename> class QVector;
template<typename> class QStack;
template<typename> class QQueue;
template<typename> class QSet;

template<typename, typename> class QMap;
template<typename, typename> class QHash;
template<typename, int> class QVarLengthArray;

namespace ALog {

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

using Buffer = std::vector<uint8_t>;

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

    inline bool value() const {
        assert(m_hasValue);
        return m_value;
    }

    bool value_or(bool value) const noexcept { return m_hasValue ? m_value : value; }

    operator bool() const noexcept { return m_hasValue; }
    bool operator*() const { return value(); }

private:
    bool m_hasValue { false };
    bool m_value;
};
#pragma pack(pop)

struct sfinae_base
{
    using yes = char;
    using no  = yes[2];
};

template <typename T>
struct has_const_iterator : private sfinae_base
{
private:
    template <typename C> static yes & test(typename C::const_iterator*);
    template <typename C> static no  & test(...);
public:
    static const bool value = sizeof(test<T>(nullptr)) == sizeof(yes);
    using type =  T;

    void dummy(); //for GCC to supress -Wctor-dtor-privacy
};

template <typename T>
struct has_begin_end : private sfinae_base
{
private:
    template <typename C>
    static yes & f(typename std::enable_if<
                   std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::begin)),
                   typename C::const_iterator(C::*)() const>::value>::type*);

    template <typename C>
    static no & f(...);

    template <typename C>
    static yes & g(typename std::enable_if<
                   std::is_same<decltype(static_cast<typename C::const_iterator(C::*)() const>(&C::end)),
                   typename C::const_iterator(C::*)() const>::value, void>::type*);

    template <typename C>
    static no & g(...);

public:
    static bool const beg_value = sizeof(f<T>(nullptr)) == sizeof(yes);
    static bool const end_value = sizeof(g<T>(nullptr)) == sizeof(yes);
    static bool const value = beg_value && end_value;

    void dummy(); //for GCC to supress -Wctor-dtor-privacy
};

template <typename T>
struct is_container : public std::integral_constant<bool,
        has_const_iterator<T>::value &&
        has_begin_end<T>::value> { };

template <std::size_t N>
struct is_container<char[N]> : std::false_type { };
template <std::size_t N>
struct is_container<wchar_t[N]> : std::false_type { };

template <> struct is_container<QString> : std::false_type { };
template <> struct is_container<QLatin1String> : std::false_type { };
template <> struct is_container<QStringRef> : std::false_type { };
template <> struct is_container<QByteArray> : std::false_type { };
template <> struct is_container<std::string> : std::false_type { };
template <> struct is_container<std::wstring> : std::false_type { };

template <typename T>
struct has_key : public std::integral_constant<bool, false> { };

template <typename... Ts> struct has_key<std::map<Ts...>> : std::true_type { };
template <typename... Ts> struct has_key<std::multimap<Ts...>> : std::true_type { };
template <typename... Ts> struct has_key<std::unordered_map<Ts...>> : std::true_type { };
template <typename... Ts> struct has_key<std::unordered_multimap<Ts...>> : std::true_type { };
template <typename... Ts> struct has_key<QMap<Ts...>> : std::true_type { };
template <typename... Ts> struct has_key<QHash<Ts...>> : std::true_type { };

template <typename T>
struct is_qt_container : public std::integral_constant<bool, false> { };
template <typename... Ts> struct is_qt_container<QMap<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QHash<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QList<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QLinkedList<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QVector<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QStack<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QQueue<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QSet<Ts...>> : std::true_type { };
template <typename... Ts> struct is_qt_container<QVarLengthArray<Ts...>> : std::true_type { };

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

} // namespace ALog
