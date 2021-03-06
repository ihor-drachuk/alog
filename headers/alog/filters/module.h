#include <alog/filter.h>

namespace ALog {
namespace Filters {

class Module : public IFilter
{
public:
    Module(const char* module, bool pass = true, Mode mode = PassOrReject);
    ~Module();

protected:
    I::optional_bool canPassImpl(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
