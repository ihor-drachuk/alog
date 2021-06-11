#pragma once
#include <memory>
#include <vector>
#include <alog/tools.h>

namespace ALog {

namespace Filters { class Chain; }
struct Record;

class IConverter
{
public:
    IConverter();
    IConverter(const IConverter&) = delete;
    IConverter& operator=(const IConverter&) = delete;
    virtual ~IConverter();

    Filters::Chain& filters();

    Buffer convert(const Buffer& data, const Record& record);

protected:
    virtual Buffer convertImpl(const Buffer& data, const Record& record) = 0;

private:
    ALOG_DECLARE_PIMPL
};

using IConverterPtr = std::shared_ptr<IConverter>;

} // namespace ALog


namespace ALog {
namespace Converters {

class Chain : public IConverter, public Internal::IChain<IConverter, Chain>
{
public:
    using Internal::IChain<IConverter, Chain>::IChain;

    Buffer convertImpl(const Buffer& data, const Record& record) override;
};

} // namespace Converters
} // namespace ALog
