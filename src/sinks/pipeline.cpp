#include <alog/sinks/pipeline.h>

namespace ALog {
namespace Sinks {

struct Pipeline::impl_t
{
    Filters::Chain filters;       // Record
    IFormatterPtr formatter;      // Record -> Buffer
    Converters::Chain converters; // Buffer
    Sinks::Chain sinks;
};


Pipeline::Pipeline()
{
    createImpl();
}

Pipeline::~Pipeline()
{
}

void Pipeline::reset()
{
    impl().filters.clear();
    impl().formatter.reset();
    impl().converters.clear();
    impl().sinks.clear();
}

Filters::Chain& Pipeline::filters()
{
    return impl().filters;
}

IFormatterPtr& Pipeline::formatter()
{
    return impl().formatter;
}

Converters::Chain& Pipeline::converters()
{
    return impl().converters;
}

Sinks::Chain& Pipeline::sinks()
{
    return impl().sinks;
}

void Pipeline::write(const Buffer& buffer, const Record& record)
{
    if (impl().sinks.empty())
        return;

    assert(buffer.size() || impl().formatter);

    if (impl().filters.canPass(record).value_or(true)) {
        if (impl().formatter) {
            impl().sinks.write(impl().converters.convert(impl().formatter->format(record)), record);
        } else {
            impl().sinks.write(impl().converters.convert(buffer), record);
        }
    }
}

void Pipeline::flush()
{
    impl().sinks.flush();
}

} // namespace Sinks
} // namespace ALog

