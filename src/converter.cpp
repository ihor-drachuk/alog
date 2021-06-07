#include <alog/converter.h>

namespace ALog {
namespace Converters {

Chain::Chain(const std::initializer_list<IConverterPtr>& filters)
{
    for (const auto& x : filters)
        m_converters.push_back(x);
}

void Chain::clear()
{
    m_converters.clear();
}

Buffer Chain::convert(const Buffer& data)
{
    if (m_converters.size() == 1)
        return m_converters.begin()->get()->convert(data);

    if (m_converters.empty())
        return data;

    Buffer buffer1;
    Buffer buffer2;
    const Buffer* in = &data;
    Buffer* out = &buffer2;
    Buffer* result = nullptr;

    auto it = m_converters.begin();

    while (it != m_converters.end()) {
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
