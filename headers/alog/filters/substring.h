/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/filter.h>

namespace ALog {
namespace Filters {

class Substring : public IFilter
{
    ALOG_NO_COPY_MOVE(Substring);
public:
    Substring(const char* substring, bool pass = true,
              bool caseSensitive = true, Mode mode = PassOrReject);
    ~Substring();

protected:
    I::optional_bool canPassImpl(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Filters
} // namespace ALog
