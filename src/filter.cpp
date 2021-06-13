#include <alog/filter.h>

#include <string>

namespace ALog {

Internal::optional_bool IFilter::canPass(const Record& record) const
{
    auto result = canPassImpl(record);

    if (!result.has_value()) {
        if (m_mode & UndefinedIsPass)
            return true;
        else if (m_mode & UndefinedIsReject)
            return false;
        else
            return {};
    }

    Mode modeStripped = static_cast<Mode>(m_mode
                        & ~UndefinedIsPass
                        & ~UndefinedIsReject);

    switch (modeStripped) {
        case PassOrReject: return *result;
        case PassOrUndefined: return *result ? I::optional_bool(true) : I::optional_bool();
        case RejectOrUndefined: return *result ? I::optional_bool() : I::optional_bool(false);

        case UndefinedIsPass: break;
        case UndefinedIsReject: break;
    }

    assert(!"Should not get here!");
    return {};
}

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
        if (result.has_value()) break;
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

Internal::optional_bool Chain_NOR::canPass(const Record& record) const
{
    if (empty()) return m_defaultDecision;

    for (const auto& x : items())
        if (x->canPass(record).value_or(false)) return false;

    return true;
}

} // namespace Filters
} // namespace ALog
