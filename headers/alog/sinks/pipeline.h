#pragma once
#include <alog/converter.h>
#include <alog/filter.h>
#include <alog/formatter.h>
#include <alog/sink.h>
#include <alog/tools.h>

namespace ALog {
namespace Sinks {

class Pipeline : public ISink
{
public:
    Pipeline();
    Pipeline(no_initialization_tag);
    ~Pipeline() override;

    void reset();

    Filters::Chain& filters();
    IFormatterPtr& formatter();
    Converters::Chain& converters();
    Sinks::Chain& sinks();

    void setBufferPipelineMode(bool enable = true);

    void write(const Buffer& buffer, const Record& record) override;
    void flush() override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Sinks
} // namespace ALog
