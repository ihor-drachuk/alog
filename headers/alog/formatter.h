#pragma once
#include <memory>
#include <alog/record.h>
#include <alog/tools.h>

namespace ALog {

class IFormatter
{
public:
    IFormatter() = default;
    IFormatter(const IFormatter&) = delete;
    IFormatter& operator=(const IFormatter&) = delete;

    virtual ~IFormatter() = default;
    virtual Buffer format(const Record& record) const = 0;
};

using IFormatterPtr = std::shared_ptr<IFormatter>;

} // namespace ALog
