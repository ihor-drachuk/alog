#include <alog/sinks/baical.h>

#ifdef ALOG_HAS_P7_LIBRARY
#include <P7_Trace.h>

#include <locale>
#include <codecvt>
#include <string>

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
        m_buffer = m_convert.from_bytes(value);
        return m_buffer.data();
    }

    const wchar_t* getBuffer() const { return m_buffer.data(); }

private:
    std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t> m_convert;
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


struct SinkBaical::impl_t
{
    IP7_Client* client { nullptr };
    IP7_Trace* trace { nullptr };
    IP7_Trace::hModule hModule { nullptr };

    const StringConv<tXCHAR> formatString {"%s"};
    StringConv<tXCHAR> messageString;
};


SinkBaical::SinkBaical()
{
    createImpl();

    impl().client = P7_Create_Client(TM("/P7.Sink=Baical /P7.Addr=127.0.0.1"));
    if (!impl().client) return;

    impl().trace =  P7_Create_Trace(impl().client, TM("Channel 1"));
    if (!impl().trace) return;

    impl().trace->Register_Module(TM(""), &impl().hModule);
}

SinkBaical::~SinkBaical()
{
    if (impl().trace)
        impl().trace->Release();

    if (impl().client)
        impl().client->Release();
}

void SinkBaical::write(const Buffer& /*buffer*/, const Record& record)
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
