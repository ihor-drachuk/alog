#include <alog/sink.h>
#include <alog/tools.h>
#include <stdexcept>

namespace ALog {
namespace Sinks {

Chain::Chain(const std::initializer_list<ISinkPtr>& filters)
{
    for (const auto& x : filters)
        m_sinks.push_back(x);
}

void Chain::clear()
{
    m_sinks.clear();
}

void Chain::write(const Buffer& buffer, const Record& record)
{
    for (auto& x : m_sinks)
        x->write(buffer, record);
}

void Chain::flush()
{
    for (auto& x : m_sinks)
        x->flush();
}

} // namespace Sinks
} // namespace ALog
