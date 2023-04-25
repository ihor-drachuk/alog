#pragma once
#include <alog/sink.h>

namespace ALog {
namespace Sinks {

class File : public ISink
{
    ALOG_NO_COPY_MOVE(File);
public:
    File(const char* fileName, bool createPath = false); // throws
    ~File() override;

    void write(const Buffer& buffer, const Record&) override;
    void flush() override;

    size_t expectedNewSize(const Buffer& buffer) const;
    size_t getSize() const { return m_size; }

private:
    Buffer m_buffer;
    FILE* m_handle { nullptr };
    size_t m_size {};
};

} // namespace Sinks
} // namespace ALog
