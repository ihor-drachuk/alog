#include <alog/filter.h>

#include <string>

namespace ALog {
namespace Filters {

void Chain::clear()
{
    Internal::IChain<IFilter, Chain>::clear();
    m_defaultDecision = true;
}

I::optional_bool Chain::canPass(const Record& record) const
{
    I::optional_bool result;

    for (const auto& x : items()) {
        result = x->canPass(record);
        if (result) break;
    }

    return result.value_or(m_defaultDecision);
}

I::optional_bool Chain_OR::canPass(const Record& record) const
{
    if (empty()) return m_defaultDecision;

    for (const auto& x : items())
        if (x->canPass(record).value_or(false)) return true;

    return false;
}

I::optional_bool Chain_AND::canPass(const Record& record) const
{
    if (empty()) return m_defaultDecision;

    for (const auto& x : items())
        if (!x->canPass(record).value_or(true)) return false;

    return true;
}

} // namespace Filters
} // namespace ALog
