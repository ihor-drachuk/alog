#include <alog/filter.h>

namespace ALog {
namespace Filters {

class SeverityModule : public IFilter
{
public:
    SeverityModule(Severity severity,
                   const char* module,
                   ALog::Comparison1 comparison = ALog::GreaterEqual);
    ~SeverityModule();
    I::optional_bool canPass(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
