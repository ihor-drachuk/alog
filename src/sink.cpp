#include <alog/sink.h>
#include <alog/tools.h>
#include <stdexcept>

namespace ALog {
namespace Sinks {

void Chain::write(const Buffer& buffer, const Record& record)
{
    for (auto& x : items())
        x->write(buffer, record);
}

void Chain::flush()
{
    for (auto& x : items())
        x->flush();
}

} // namespace Sinks
} // namespace ALog
