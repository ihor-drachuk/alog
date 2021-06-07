#pragma once
#include <memory>
#include <vector>
#include <alog/record.h>
#include <alog/tools.h>

namespace ALog {

class IFilter
{
public:
    virtual ~IFilter() = default;
    virtual I::optional_bool canPass(const Record& record) const = 0;
};

using IFilterPtr = std::shared_ptr<IFilter>;

} // namespace ALog


namespace ALog {
namespace Filters {

class Chain : public IFilter
{
public:
    Chain() = default;
    Chain(const std::initializer_list<IFilterPtr>& filters);

    void setDefaultDecision(bool value) { m_defaultDecision = value; }

    Chain& addFilter(const IFilterPtr& filter);
    void clear();
    inline bool empty() const { return m_filters.empty(); };

    I::optional_bool canPass(const Record& record) const override;

protected:
    bool m_defaultDecision { true };
    std::vector<IFilterPtr> m_filters;
};


class Chain_OR : public Chain
{
public:
    using Chain::Chain;
    I::optional_bool canPass(const Record& record) const override;
};


class Chain_AND : public Chain
{
public:
    using Chain::Chain;
    I::optional_bool canPass(const Record& record) const override;
};


template<typename T>
class Functor : public IFilter
{
public:
    Functor(const T& func): m_func(func) { }
    I::optional_bool canPass(const Record& record) const override { return m_func(record); }

private:
    T m_func;
};

using Functor2 = Functor<std::function<I::optional_bool(const Record&)>>;

} // namespace Filters
} // namespace ALog
