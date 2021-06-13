#include <alog/filters/severity_module.h>

namespace ALog {
namespace Filters {

struct SeverityModule::impl_t
{
    Severity severity;
    std::string module;
    ALog::Comparison1 comparison;
};

SeverityModule::SeverityModule(::ALog::Severity severity, const char* module, Mode mode, Comparison1 comparison)
    : IFilter(mode)
{
    createImpl();
    impl().severity = severity;
    impl().module = module;
    impl().comparison = comparison;
}

SeverityModule::~SeverityModule()
{
}

I::optional_bool SeverityModule::canPassImpl(const Record& record) const
{
    if (impl().module != record.module) return {};
    auto ge = record.severity >= impl().severity;
    return (impl().comparison == ALog::GreaterEqual) ? ge : !ge;
}

} // namespace Filters
} // namespace ALog
