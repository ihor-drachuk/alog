/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

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
