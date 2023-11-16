/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/converters/attention.h>

namespace ALog {
namespace Converters {

Buffer Attention::convertImpl(const Buffer& data, const Record& /*record*/)
{
    const auto str = "  !!!  ";
    const size_t sz = strlen(str);

    Buffer buf(data.size() + sz);
    memcpy(buf.data(), str, sz);
    memcpy(buf.data() + sz, data.data(), data.size());

    return buf;
}

} // namespace Converters
} // namespace ALog
