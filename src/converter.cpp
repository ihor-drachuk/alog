/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/converter.h>

#include <alog/filter.h>

namespace ALog {

struct IConverter::impl_t
{
    Filters::Chain filters;
};

IConverter::IConverter()
{
    createImpl();
}

IConverter::~IConverter()
{
}

Filters::Chain& IConverter::filters()
{
    return impl().filters;
}

Buffer IConverter::convert(const Buffer& data, const Record& record)
{
    return impl().filters.canPass(record).value_or(true) ?
                convertImpl(data, record) :
                data;
}

namespace Converters {

Buffer Chain::convertImpl(const Buffer& data, const Record& record)
{
    if (items().size() == 1)
        return items().begin()->get()->convert(data, record);

    if (items().empty())
        return data;

    Buffer buffer1;
    Buffer buffer2;
    const Buffer* in = &data;
    Buffer* out = &buffer2;
    Buffer* result = nullptr;

    auto it = items().begin();

    while (it != items().end()) {
        *out = it->get()->convert(*in, record);
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
