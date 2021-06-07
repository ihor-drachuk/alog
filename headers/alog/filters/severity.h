#include <alog/filter.h>

namespace ALog {
namespace Filters {

class Severity : public IFilter
{
public:
    Severity(::ALog::Severity severity, ALog::Comparison1 comparison = ALog::GreaterEqual)
        : m_severity(severity),
          m_comparison(comparison)
    { }

    I::optional_bool canPass(const Record& record) const override;

private:
    ::ALog::Severity m_severity;
    ALog::Comparison1 m_comparison;
};

} // namespace Filters
} // namespace ALog
