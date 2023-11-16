/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/formatter.h>

namespace ALog {
namespace Formatters {

class Minimal : public IFormatter
{
public:
    Buffer format(const Record& record) const override;
};

} // namespace Formatters
} // namespace ALog
