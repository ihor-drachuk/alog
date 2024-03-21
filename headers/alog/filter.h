#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <alog/record.h>
#include <alog/tools.h>

namespace ALog {

class IFilter
{
public:
    enum Mode {
        PassOrReject,
        PassOrUndefined,
        RejectOrUndefined,

        Default = PassOrReject,

        UndefinedIsPass = 1024,
        UndefinedIsReject = 2048,
    };

    IFilter(Mode mode = PassOrReject): m_mode(mode) { };
    IFilter(const IFilter&) = delete;
    IFilter& operator=(const IFilter&) = delete;

    virtual ~IFilter() = default;
    virtual I::optional_bool canPass(const Record& record) const;

protected:
    virtual I::optional_bool canPassImpl(const Record& record) const { (void)record; return {}; };

protected:
    Mode m_mode;
};

using IFilterPtr = std::shared_ptr<IFilter>;

} // namespace ALog


namespace ALog {
namespace Filters {

class Chain : public IFilter, public Internal::IChain<IFilter, Chain>
{
public:
    using Internal::IChain<IFilter, Chain>::IChain;

    void setDefaultDecision(bool value) { m_defaultDecision = value; }

    I::optional_bool canPass(const Record& record) const override;

    void clear() override;

protected:
    bool m_defaultDecision { true };
};


class Chain_OR : public Chain
{
public:
    using Chain_OR_Ptr = std::shared_ptr<Chain_OR>;

public:
    using Chain::Chain;

    template<typename... Args>
    [[nodiscard]] static Chain_OR_Ptr create(Args... args) { return std::make_shared<Chain_OR>(std::forward<Args>(args)...); }
    [[nodiscard]] static Chain_OR_Ptr create(const std::initializer_list<IFilterPtr>& filters) { return std::make_shared<Chain_OR>(filters); }

    I::optional_bool canPass(const Record& record) const override;
};


class Chain_AND : public Chain
{
public:
    using Chain_AND_Ptr = std::shared_ptr<Chain_AND>;

public:
    using Chain::Chain;

    template<typename... Args>
    [[nodiscard]] static Chain_AND_Ptr create(Args... args) { return std::make_shared<Chain_AND>(std::forward<Args>(args)...); }
    [[nodiscard]] static Chain_AND_Ptr create(const std::initializer_list<IFilterPtr>& filters) { return std::make_shared<Chain_AND>(filters); }

    I::optional_bool canPass(const Record& record) const override;
};


class Chain_NOR : public Chain
{
public:
    using Chain_NOR_Ptr = std::shared_ptr<Chain_NOR>;

public:
    using Chain::Chain;

    template<typename... Args>
    [[nodiscard]] static Chain_NOR_Ptr create(Args... args) { return std::make_shared<Chain_NOR>(std::forward<Args>(args)...); }
    [[nodiscard]] static Chain_NOR_Ptr create(const std::initializer_list<IFilterPtr>& filters) { return std::make_shared<Chain_NOR>(filters); }

    I::optional_bool canPass(const Record& record) const override;
};


template<typename T>
class Functor : public IFilter
{
public:
    Functor(const T& func, Mode mode = PassOrReject)
        : IFilter(mode),
          m_func(func)
    { }

protected:
    I::optional_bool canPassImpl(const Record& record) const override { return m_func(record); }

private:
    T m_func;
};

using Functor2 = Functor<std::function<I::optional_bool(const Record&)>>;


class Always : public IFilter
{
public:
    Always(I::optional_bool pass): m_pass(pass) { }
    I::optional_bool canPass(const Record&) const override { return m_pass; }

private:
    I::optional_bool m_pass;
};

} // namespace Filters
} // namespace ALog
