#include <alog/filter.h>

#include <string>

namespace ALog {

I::optional_bool FilterSeverity::canPass(const Record& record) const
{
    return record.severity >= m_severity;
}

FilterStorage::FilterStorage(const std::initializer_list<IFilterPtr>& filters)
{
    for (const auto& x : filters)
        m_filters.push_back(x);
}

FilterStorage& FilterStorage::addFilter(const IFilterPtr& filter)
{
    m_filters.push_back(filter);
    return *this;
}

I::optional_bool FilterStorage::canPass(const Record& record) const
{
    I::optional_bool result;

    for (const auto& x : m_filters) {
        result = x->canPass(record);
        if (result) break;
    }

    return result.value_or(m_defaultDecision);
}

I::optional_bool FilterStorage_OR::canPass(const Record& record) const
{
    if (m_filters.empty()) return m_defaultDecision;

    for (const auto& x : m_filters)
        if (x->canPass(record).value_or(false)) return true;

    return false;
}

I::optional_bool FilterStorage_AND::canPass(const Record& record) const
{
    if (m_filters.empty()) return m_defaultDecision;

    for (const auto& x : m_filters)
        if (!x->canPass(record).value_or(true)) return false;

    return true;
}

struct FilterModuleSeverity::impl_t
{
    Severity severity;
    std::string module;
};

FilterModuleSeverity::FilterModuleSeverity(Severity severity, const char* module)
{
    createImpl();
    impl().severity = severity;
    impl().module = module;
}

FilterModuleSeverity::~FilterModuleSeverity()
{
}

I::optional_bool FilterModuleSeverity::canPass(const Record& record) const
{
    if (impl().module != record.module) return {};
    return record.severity >= impl().severity;
}

} // namespace ALog
