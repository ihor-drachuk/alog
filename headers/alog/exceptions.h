/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#pragma once
#include <string>
#include <stdexcept>

namespace ALog {

class runtime_error_wide : public std::runtime_error
{
public:
    runtime_error_wide(const char* msg);

private:
    std::wstring m_msg;
};

} // namespace ALog
