#include <alog/filters/file.h>

namespace ALog {
namespace Filters {

struct File::impl_t
{
    std::string file;
    bool pass;
};

File::File(const char* file, bool pass, Mode mode)
    : IFilter(mode)
{
    createImpl();
    impl().file = file;
    impl().pass = pass;
}

File::~File()
{
}

I::optional_bool File::canPassImpl(const Record& record) const
{
    return !((impl().file == record.filenameOnly) ^ impl().pass);
}

} // namespace Filters
} // namespace ALog
