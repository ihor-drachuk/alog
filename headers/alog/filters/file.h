/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/filter.h>

namespace ALog {
namespace Filters {

class File : public IFilter
{
    ALOG_NO_COPY_MOVE(File);
public:
    File(const char* file, bool pass = true, Mode mode = PassOrReject);
    ~File();

protected:
    I::optional_bool canPassImpl(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
