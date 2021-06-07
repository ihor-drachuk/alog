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
