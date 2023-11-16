/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

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
    ALOG_NO_COPY_MOVE(Pipeline);
public:
    Pipeline();
    ~Pipeline() override;

    void reset();

    Filters::Chain& filters();
    IFormatterPtr& formatter();
    Converters::Chain& converters();
    Sinks::Chain& sinks();

    void write(const Buffer& buffer, const Record& record) override;
    void flush() override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Sinks
} // namespace ALog
