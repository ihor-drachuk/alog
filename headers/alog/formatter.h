#pragma once
#include <memory>
#include <alog/record.h>
#include <alog/tools.h>

namespace ALog {

class IFormatter
{
public:
    virtual ~IFormatter() = default;
    virtual Buffer format(const Record& record) const = 0;
};

using IFormatterPtr = std::shared_ptr<IFormatter>;

} // namespace ALog
