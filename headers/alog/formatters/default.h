/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/formatter.h>

namespace ALog {
namespace Formatters {

class Default : public IFormatter
{
    ALOG_NO_COPY_MOVE(Default);
public:
    Default();
    ~Default() override;
    Buffer format(const Record& record) const override;

private:
    ALOG_DECLARE_PIMPL
};

} // namespace Formatters
} // namespace ALog
