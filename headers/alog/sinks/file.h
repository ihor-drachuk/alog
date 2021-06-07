#pragma once
#include <alog/sink.h>

namespace ALog {
namespace Sinks {

class File : public ISink
{
public:
    File(const char* fileName);
    ~File() override;

    void write(const Buffer& buffer, const Record& record) override;
    void flush() override;

private:
    Buffer m_buffer;
    FILE* m_handle { nullptr };
};

} // namespace Sinks
} // namespace ALog
