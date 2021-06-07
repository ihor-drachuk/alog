#include <alog/filter.h>

namespace ALog {
namespace Filters {

class SeverityFile : public IFilter
{
public:
    SeverityFile(::ALog::Severity severity,
                 const std::string& fileName,
                 ALog::Comparison1 comparison = ALog::GreaterEqual);
    ~SeverityFile();
    I::optional_bool canPass(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
