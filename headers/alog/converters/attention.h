/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

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
