#include <alog/filters/severity_file.h>

namespace ALog {
namespace Filters {

struct SeverityFile::impl_t
{
    Severity severity;
    std::string fileName;
    ALog::Comparison1 comparison;
};

SeverityFile::SeverityFile(::ALog::Severity severity, const std::string& fileName, Mode mode, Comparison1 comparison)
    : IFilter(mode)
{
    createImpl();
    impl().severity = severity;
    impl().fileName = fileName;
    impl().comparison = comparison;
}

SeverityFile::~SeverityFile()
{
}

I::optional_bool SeverityFile::canPassImpl(const Record& record) const
{
    if (impl().fileName != record.filenameOnly) return {};
    auto ge = record.severity >= impl().severity;
    return (impl().comparison == ALog::GreaterEqual) ? ge : !ge;
}

} // namespace Filters
} // namespace ALog
