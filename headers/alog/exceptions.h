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
