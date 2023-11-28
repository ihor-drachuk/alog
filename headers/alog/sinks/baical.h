/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#include <alog/sink.h>
#include <alog/record.h>

namespace ALog {
namespace Sinks {

#ifdef ALOG_HAS_P7_LIBRARY

#define ALOG_HAS_SINK_BAICAL 1

class Baical : public ISink
{
public:
    Baical();
    ~Baical() override;

    void write(const Buffer& buffer, const Record& record) override;

private:
    ALOG_DECLARE_PIMPL
};

#endif // ALOG_HAS_P7_LIBRARY

} // namespace Sinks
} // namespace ALog
