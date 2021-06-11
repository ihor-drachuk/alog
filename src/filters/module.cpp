#include <alog/filters/module.h>

namespace ALog {
namespace Filters {

struct Module::impl_t
{
    std::string module;
    bool pass;
};

Module::Module(const char* module, bool pass)
{
    createImpl();
    impl().module = module;
    impl().pass = pass;
}

Module::~Module()
{
}

I::optional_bool Module::canPass(const Record& record) const
{
    bool match = (impl().module == record.module);
    return impl().pass ? match : !match;
}

} // namespace Filters
} // namespace ALog
