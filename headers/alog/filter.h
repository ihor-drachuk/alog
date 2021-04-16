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
    virtual optional_bool canPass(const Record& Record) const = 0;
};

using IFilterPtr = std::shared_ptr<IFilter>;

class FilterSeverity : public IFilter
{
public:
    FilterSeverity(Severity severity): m_severity(severity) { }
    optional_bool canPass(const Record& Record) const override;

private:
    Severity m_severity;
};

class FilterModuleSeverity : public IFilter
{
public:
    FilterModuleSeverity(Severity Severity, const char* module);
    ~FilterModuleSeverity();
    optional_bool canPass(const Record& Record) const override;

private:
    ALOG_DECLARE_PIMPL
};

class FilterStorage : public IFilter
{
public:
    FilterStorage() = default;
    FilterStorage(const std::initializer_list<IFilterPtr>& filters);

    void setDefaultDecision(bool value) { m_defaultDecision = value; }
    FilterStorage& addFilter(const IFilterPtr& filter);
    optional_bool canPass(const Record& Record) const override;

protected:
    bool m_defaultDecision { true };
    std::vector<IFilterPtr> m_filters;
};

class FilterStorage_OR : public FilterStorage
{
public:
    using FilterStorage::FilterStorage;
    optional_bool canPass(const Record& Record) const override;
};

class FilterStorage_AND : public FilterStorage
{
public:
    using FilterStorage::FilterStorage;
    optional_bool canPass(const Record& Record) const override;
};

} // namespace ALog
