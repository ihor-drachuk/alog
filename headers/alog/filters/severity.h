/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/filter.h>

namespace ALog {
namespace Filters {

class Severity : public IFilter
{
public:
    Severity(::ALog::Severity severity, Mode mode = PassOrReject, ALog::Comparison1 comparison = ALog::GreaterEqual)
        : IFilter(mode),
          m_severity(severity),
          m_comparison(comparison)
    { }


protected:
    I::optional_bool canPassImpl(const Record& record) const override;

private:
    ::ALog::Severity m_severity;
    ALog::Comparison1 m_comparison;
};

} // namespace Filters
} // namespace ALog
