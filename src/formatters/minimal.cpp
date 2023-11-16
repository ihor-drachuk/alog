/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/formatters/minimal.h>

namespace ALog {
namespace Formatters {

Buffer Minimal::format(const Record& record) const
{
    Buffer resultBuffer(record.getMessageLen());
    memcpy(resultBuffer.data(), record.getMessage(), record.getMessageLen());
    return resultBuffer;
}

} // namespace Formatters
} // namespace ALog
