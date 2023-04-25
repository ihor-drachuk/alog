#include <alog/sinks/file.h>

#include <alog/tools_internal.h>
#include <alog/tools_filesystem.h>
#include <stdexcept>
#include <cassert>

namespace ALog {
namespace Sinks {

File::File(const char* fileName, bool createPath)
{
    assert(std::filesystem::path(fileName).has_filename());

    if (createPath)
        std::filesystem::create_directories(std::filesystem::path(fileName).remove_filename()); // throws

    m_size = Internal::getFileSize(fileName).value_or(0);
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

size_t File::expectedNewSize(const Buffer& buffer) const
{
    return m_size + buffer.size() + 1;
}

void File::write(const Buffer& buffer, const Record&)
{
#ifndef NDEBUG // Is debug
    const auto expectedSize = expectedNewSize(buffer);
#endif // !NDEBUG (Is debug)

    const auto sz = buffer.size();

    m_buffer.resize(sz + 1);
    memcpy(m_buffer.data(), buffer.data(), sz);
    m_buffer.back() = '\n';

    m_size += fwrite(m_buffer.data(), 1, m_buffer.size(), m_handle); // error not handled
    assert(m_size == expectedSize);
}

} // namespace Sinks
} // namespace ALog
