/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/sinks/baical.h>

#ifdef ALOG_HAS_P7_LIBRARY
#include <P7_Trace.h>

#include <string>
#include <cwchar>

namespace ALog {
namespace Sinks {

static_assert (sizeof(char) == 1, "Unexpected char size!");
static_assert (sizeof(wchar_t) == 2, "Unexpected wchar_t size!");

template<typename T>
struct StringConv { StringConv() = delete; };

template<>
struct StringConv<wchar_t> {
    StringConv() { }
    StringConv(const char* value) { convert(value); }

    const wchar_t* convert(const char* value) {
        std::mbstate_t state = std::mbstate_t();
        const char* srcPtr = value;
        const size_t srcLen = strlen(srcPtr);

        size_t dstLen {0}; // Length without \0

#ifdef ALOG_COMPILER_MSVC
        mbsrtowcs_s(&dstLen, nullptr, 0, &srcPtr, srcLen, &state);
        dstLen--;
#elif ALOG_COMPILER_GCC || ALOG_COMPILER_CLANG
        dstLen = mbsrtowcs(nullptr, &srcPtr, srcLen, &state);
#else
        #error "Unsupported compiler"
#endif

        m_buffer.resize(dstLen);

#ifdef ALOG_COMPILER_MSVC
        mbsrtowcs_s(&dstLen, m_buffer.data(), dstLen+1, &srcPtr, srcLen, &state);
#elif ALOG_COMPILER_GCC || ALOG_COMPILER_CLANG
        mbsrtowcs(m_buffer.data(), &srcPtr, srcLen, &state);
#else
        #error "Unsupported compiler"
#endif

        return m_buffer.data();
    }

    const wchar_t* getBuffer() const { return m_buffer.data(); }

private:
    std::wstring m_buffer;
};

template<>
struct StringConv<char> {
    StringConv() { }
    StringConv(const char* value) { convert(value); }

    const char* convert(const char* value) {
        m_buffer = value;
        return m_buffer;
    }

    const char* getBuffer() const { return m_buffer; }

private:
    const char* m_buffer;
};


struct Baical::impl_t
{
    IP7_Client* client { nullptr };
    IP7_Trace* trace { nullptr };
    IP7_Trace::hModule hModule { nullptr };

    const StringConv<tXCHAR> formatString {"%s"};
    StringConv<tXCHAR> messageString;
};


Baical::Baical()
{
    createImpl();

    impl().client = P7_Create_Client(TM("/P7.Sink=Baical /P7.Addr=127.0.0.1"));
    if (!impl().client) return;

    impl().trace =  P7_Create_Trace(impl().client, TM("Channel 1"));
    if (!impl().trace) return;

    impl().trace->Register_Module(TM(""), &impl().hModule);
}

Baical::~Baical()
{
    if (impl().trace)
        impl().trace->Release();

    if (impl().client)
        impl().client->Release();
}

void Baical::write(const Buffer& /*buffer*/, const Record& record)
{
    if (!impl().client || !impl().trace) return;

    auto format = impl().formatString.getBuffer();
    auto msg = impl().messageString.convert(record.getMessage());

    impl().trace->Trace(0,
                        (eP7Trace_Level)record.severity,
                        impl().hModule,
                        record.line,
                        record.filenameFull,
                        record.func,
                        format,
                        msg);
}


} // namespace Sinks
} // namespace ALog

#endif // ALOG_HAS_P7_LIBRARY
