#pragma once
#include <memory>
#include <alog/tools.h>

namespace ALog {

class IConverter
{
public:
    virtual ~IConverter() = default;
    virtual Buffer convert(const Buffer& data) = 0;
};

using IConverterPtr = std::shared_ptr<IConverter>;

} // namespace ALog
