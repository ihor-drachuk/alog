#pragma once
#include <memory>
#include <vector>
#include <alog/tools.h>

namespace ALog {

class IConverter
{
public:
    IConverter() = default;
    IConverter(const IConverter&) = delete;
    IConverter& operator=(const IConverter&) = delete;

    virtual ~IConverter() = default;
    virtual Buffer convert(const Buffer& data) = 0;
};

using IConverterPtr = std::shared_ptr<IConverter>;

} // namespace ALog


namespace ALog {
namespace Converters {

class Chain : public IConverter, public Internal::IChain<IConverter, Chain>
{
public:
    using Internal::IChain<IConverter, Chain>::IChain;

    Buffer convert(const Buffer& data) override;
};

} // namespace Converters
} // namespace ALog
