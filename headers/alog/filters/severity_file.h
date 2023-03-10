#include <alog/filter.h>

namespace ALog {
namespace Filters {

class SeverityFile : public IFilter
{
    ALOG_NO_COPY_MOVE(SeverityFile);
public:
    SeverityFile(::ALog::Severity severity,
                 const std::string& fileName,
                 Mode mode = PassOrReject,
                 ALog::Comparison1 comparison = ALog::GreaterEqual);
    ~SeverityFile();

protected:
    I::optional_bool canPassImpl(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
