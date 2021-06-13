#include <alog/filters/module.h>

namespace ALog {
namespace Filters {

struct Module::impl_t
{
    std::string module;
    bool pass;
};

Module::Module(const char* module, bool pass, Mode mode)
    : IFilter(mode)
{
    createImpl();
    impl().module = module;
    impl().pass = pass;
}

Module::~Module()
{
}

I::optional_bool Module::canPassImpl(const Record& record) const
{
    return !((impl().module == record.module) ^ impl().pass);
}

} // namespace Filters
} // namespace ALog
