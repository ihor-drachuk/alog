#include <alog/sink.h>
#include <alog/record.h>

namespace ALog {

#ifdef ALOG_HAS_P7_LIBRARY

#define ALOG_HAS_SINK_BAICAL 1

class SinkBaical : public ISink
{
public:
    SinkBaical();
    ~SinkBaical() override;

    void write(const Buffer& buffer, const Record& record) override;

private:
    ALOG_DECLARE_PIMPL
};

#endif // ALOG_HAS_P7_LIBRARY

} // namespace ALog
