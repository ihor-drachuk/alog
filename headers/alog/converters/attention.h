#pragma once
#include <alog/converter.h>

namespace ALog {
namespace Converters {

class Attention : public IConverter
{
public:
    Buffer convert(const Buffer& data) override;
};

} // namespace Converters
} // namespace ALog
