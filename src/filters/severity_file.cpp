#include <alog/filters/severity_file.h>

namespace ALog {
namespace Filters {

struct SeverityFile::impl_t
{
    Severity severity;
    std::string fileName;
    ALog::Comparison1 comparison;
};

SeverityFile::SeverityFile(Severity severity, const std::string& fileName, Comparison1 comparison)
{
    createImpl();
    impl().severity = severity;
    impl().fileName = fileName;
    impl().comparison = comparison;
}

SeverityFile::~SeverityFile()
{
}

I::optional_bool SeverityFile::canPass(const Record& record) const
{
    if (!strstr(record.filenameOnly, impl().fileName.c_str())) return {};
    auto ge = record.severity >= impl().severity;
    return (impl().comparison == ALog::GreaterEqual) ? ge : !ge;
}

} // namespace Filters
} // namespace ALog
