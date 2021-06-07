#include <alog/sinks/file.h>

#include <stdexcept>

namespace ALog {
namespace Sinks {

File::File(const char* fileName)
{
    m_handle = fopen(fileName, "ab+");

    if (!m_handle)
        throw std::runtime_error("Can't create SinkSimpleFile");
}

File::~File()
{
    fclose(m_handle);
}

void File::flush()
{
    fflush(m_handle);
}

void File::write(const Buffer& buffer, const Record&)
{
    const auto sz = buffer.size();

    m_buffer.resize(sz + 1);
    memcpy(m_buffer.data(), buffer.data(), sz);

    *(char*)(m_buffer.data() + sz) = '\n';

    fwrite((const char*)m_buffer.data(), 1, m_buffer.size(), m_handle);
}

} // namespace Sinks
} // namespace ALog
