#include <alog/filter.h>

#include <string>

namespace ALog {
namespace Filters {

Chain::Chain(const std::initializer_list<IFilterPtr>& filters)
{
    for (const auto& x : filters)
        m_filters.push_back(x);
}

Chain& Chain::addFilter(const IFilterPtr& filter)
{
    m_filters.push_back(filter);
    return *this;
}

void Chain::clear()
{
    m_filters.clear();
    m_defaultDecision = true;
}

I::optional_bool Chain::canPass(const Record& record) const
{
    I::optional_bool result;

    for (const auto& x : m_filters) {
        result = x->canPass(record);
        if (result) break;
    }

    return result.value_or(m_defaultDecision);
}

I::optional_bool Chain_OR::canPass(const Record& record) const
{
    if (m_filters.empty()) return m_defaultDecision;

    for (const auto& x : m_filters)
        if (x->canPass(record).value_or(false)) return true;

    return false;
}

I::optional_bool Chain_AND::canPass(const Record& record) const
{
    if (m_filters.empty()) return m_defaultDecision;

    for (const auto& x : m_filters)
        if (!x->canPass(record).value_or(true)) return false;

    return true;
}

} // namespace Filters
} // namespace ALog
