#include <alog/converter.h>

namespace ALog {
namespace Converters {

Buffer Chain::convert(const Buffer& data)
{
    if (items().size() == 1)
        return items().begin()->get()->convert(data);

    if (items().empty())
        return data;

    Buffer buffer1;
    Buffer buffer2;
    const Buffer* in = &data;
    Buffer* out = &buffer2;
    Buffer* result = nullptr;

    auto it = items().begin();

    while (it != items().end()) {
        *out = it->get()->convert(*in);
        result = out;

        if (in == &data)
            in = &buffer1;

        std::swap(const_cast<Buffer*&>(in), const_cast<Buffer*&>(out));
    }

    assert(result);

    return *result;
}

} // namespace Converters
} // namespace ALog
