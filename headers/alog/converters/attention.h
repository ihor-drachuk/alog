#pragma once
#include <alog/converter.h>

namespace ALog {
namespace Converters {

class Attention : public IConverter
{
protected:
    Buffer convertImpl(const Buffer& data, const Record& record) override;
};

} // namespace Converters
} // namespace ALog
