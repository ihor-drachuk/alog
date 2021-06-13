#include <alog/filters/severity.h>

namespace ALog {
namespace Filters {

I::optional_bool Severity::canPassImpl(const Record& record) const
{
    auto ge = record.severity >= m_severity;
    return (m_comparison == ALog::GreaterEqual) ? ge : !ge;
}

} // namespace Filters
} // namespace ALog
