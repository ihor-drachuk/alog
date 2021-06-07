#include <alog/converters/attention.h>

namespace ALog {
namespace Converters {

Buffer Attention::convert(const Buffer& data)
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
