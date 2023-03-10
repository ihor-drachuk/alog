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
