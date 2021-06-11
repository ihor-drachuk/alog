#include <alog/filter.h>

namespace ALog {
namespace Filters {

class Module : public IFilter
{
public:
    Module(const char* module, bool pass = true);
    ~Module();
    I::optional_bool canPass(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
