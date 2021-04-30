#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <alog/tools.h>
#include <alog/filter.h>
#include <alog/formatter.h>
#include <alog/converter.h>

namespace ALog {

class ISink
{
public:
    virtual ~ISink() = default;
    virtual void write(const Record& record) = 0;
    virtual void flush() { }
};

using ISinkPtr = std::shared_ptr<ISink>;

class SinkContainer : public ISink
{
public:
    void write(const Record& record) override;
    void flush() override;
    void addSink(const ISinkPtr& sink);

private:
    std::vector<ISinkPtr> m_sinks;
};

class ISinkWithFmtCnv : public ISink
{
public:
    ISinkWithFmtCnv() { m_formatter = std::make_shared<DefaultFormatter>(); }
    void setFormatter(const IFormatterPtr& formatter) { m_formatter = formatter; }
    void setConverter(const IConverterPtr& converter) { m_converter = converter; }
    void write(const Record& record) override;

protected:
    virtual void writeBuffer(const Buffer& buffer) = 0;
    const Record* getCurrentRecord() const { return m_currentRecord; }

private:
    const Record* m_currentRecord { nullptr };
    IFormatterPtr m_formatter;
    IConverterPtr m_converter;
};

class SinkWithFilter : public ISink
{
public:
    SinkWithFilter(const ISinkPtr& sink, const IFilterPtr& filter): m_sink(sink), m_filter(filter) { }
    void setSink(const ISinkPtr& sink) { m_sink = sink; }
    void setFilter(const IFilterPtr& filter) { m_filter = filter; }
    void write(const Record& record) override;
    void flush() override;

private:
    ISinkPtr m_sink;
    IFilterPtr m_filter;
};

template<typename T>
class SinkFunctor : public ISink
{
public:
    SinkFunctor(const T& func): m_func(func) { }
    void write(const Record& record) override { m_func(record); }

private:
    T m_func;
};

using SinkFunctor2 = SinkFunctor<std::function<void(const Record&)>>;

class SinkNull : public ISink
{
public:
    void write(const Record&) override { }
};

class SinkStdStream : public ISinkWithFmtCnv
{
public:
    enum class Stream {
        StdOut,
        StdErr
    };

    SinkStdStream(Stream stream = Stream::StdOut);

protected:
    void writeBuffer(const Buffer& buffer) override;

private:
    Buffer m_buffer;
    FILE* m_handle { nullptr };
};

class SinkSimpleFile : public ISinkWithFmtCnv
{
public:
    SinkSimpleFile(const char* fileName);
    ~SinkSimpleFile() override;
    void flush() override;

protected:
    void writeBuffer(const Buffer& buffer) override;

private:
    Buffer m_buffer;
    FILE* m_handle { nullptr };
};

} // namespace ALog
