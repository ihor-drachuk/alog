/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/filter.h>

namespace ALog {
namespace Filters {

class SeverityModule : public IFilter
{
    ALOG_NO_COPY_MOVE(SeverityModule);
public:
    SeverityModule(::ALog::Severity severity,
                   const char* module,
                   Mode mode = PassOrReject,
                   ALog::Comparison1 comparison = ALog::GreaterEqual);
    ~SeverityModule() override;

protected:
    I::optional_bool canPassImpl(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
