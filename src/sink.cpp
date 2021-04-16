#include <alog/sink.h>
#include <alog/tools.h>

namespace ALog {

void ISinkWithFmtCnv::write(const Record& record)
{
    assert(m_formatter);

    m_currentRecord = &record;

    writeBuffer( m_converter ? m_converter->convert(m_formatter->format(record)) :
                               m_formatter->format(record) );
}

void SinkWithFilter::write(const Record& record)
{
    assert(m_sink && m_filter);

    if (m_filter->canPass(record))
        m_sink->write(record);
}

void SinkWithFilter::flush()
{
    m_sink->flush();
}

void SinkContainer::write(const Record& record)
{
    for (auto& x : m_sinks)
        x->write(record);
}

void SinkContainer::flush()
{
    for (auto& x : m_sinks)
        x->flush();
}

void SinkContainer::addSink(const ISinkPtr& sink)
{
    m_sinks.push_back(sink);
}

SinkStdStream::SinkStdStream(SinkStdStream::Stream stream)
{
    switch (stream) {
        case Stream::StdOut:
            m_handle = stdout;
            break;

        case Stream::StdErr:
            m_handle = stderr;
            break;
    }
}

void SinkStdStream::writeBuffer(const Buffer& buffer)
{
    const auto sz = buffer.size();

    m_buffer.resize(sz + 1);
    memcpy(m_buffer.data(), buffer.data(), sz);

    *(char*)(m_buffer.data() + sz) = '\n';

    fwrite((const char*)m_buffer.data(), 1, m_buffer.size(), m_handle);
    fflush(m_handle); // Flush always
}

SinkSimpleFile::SinkSimpleFile(const char* fileName)
{
    m_handle = fopen(fileName, "ab+");

    if (!m_handle)
        throw std::runtime_error("Can't create SinkSimpleFile");
}

SinkSimpleFile::~SinkSimpleFile()
{
    fclose(m_handle);
}

void SinkSimpleFile::flush()
{
    fflush(m_handle);
}

void SinkSimpleFile::writeBuffer(const Buffer& buffer)
{
    const auto sz = buffer.size();

    m_buffer.resize(sz + 1);
    memcpy(m_buffer.data(), buffer.data(), sz);

    *(char*)(m_buffer.data() + sz) = '\n';

    fwrite((const char*)m_buffer.data(), 1, m_buffer.size(), m_handle);
}

} // namespace ALog
