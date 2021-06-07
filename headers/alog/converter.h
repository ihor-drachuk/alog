#pragma once
#include <memory>
#include <vector>
#include <alog/tools.h>

namespace ALog {

class IConverter
{
public:
    virtual ~IConverter() = default;
    virtual Buffer convert(const Buffer& data) = 0;
};

using IConverterPtr = std::shared_ptr<IConverter>;

} // namespace ALog


namespace ALog {
namespace Converters {

class Chain : public IConverter
{
public:
    Chain() = default;
    Chain(const std::initializer_list<IConverterPtr>& filters);

    Chain& addConverter(const IConverterPtr& converter) { m_converters.push_back(converter); return *this; }
    void clear();
    inline bool empty() const { return m_converters.empty(); };

    Buffer convert(const Buffer& data) override;

private:
    std::vector<IConverterPtr> m_converters;
};

} // namespace Converters
} // namespace ALog
