/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/exceptions.h>

#include <locale>
#include <codecvt>

namespace ALog {

runtime_error_wide::runtime_error_wide(const char* msg)
    : std::runtime_error(msg)
{
    m_msg = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>,wchar_t>{}.from_bytes(msg);
}

} // namespace ALog
