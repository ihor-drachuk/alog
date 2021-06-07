#include <alog/sinks/pipeline.h>

#include <alog/formatters/default.h>
#include <alog/sinks/console.h>

namespace ALog {
namespace Sinks {

struct Pipeline::impl_t
{
    Filters::Chain filters;       // Record
    IFormatterPtr formatter;      // Record -> Buffer
    Converters::Chain converters; // Buffer
    Sinks::Chain sinks;

    bool bufferPipelineMode { false };
};


Pipeline::Pipeline()
{
    createImpl();
    impl().formatter = std::make_shared<Formatters::Default>();
    impl().sinks.addSink(std::make_shared<Sinks::Console>(Sinks::Console::Stream::StdOut));
}

Pipeline::Pipeline(no_initialization_tag)
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

void Pipeline::setBufferPipelineMode(bool enable)
{
    impl().bufferPipelineMode = enable;
}

void Pipeline::write(const Buffer& buffer, const Record& record)
{
    if (impl().bufferPipelineMode) {
        // Buffer
        assert(buffer.size());
        assert(impl().filters.empty());
        assert(!impl().formatter);

        impl().sinks.write(impl().converters.convert(buffer), record);

    } else {
        // Record
        assert(buffer.empty());
        assert(impl().formatter);

        if (impl().filters.canPass(record).value_or(true))
            impl().sinks.write(impl().converters.convert(impl().formatter->format(record)), record);
    }
}

void Pipeline::flush()
{
    impl().sinks.flush();
}

} // namespace Sinks
} // namespace ALog

