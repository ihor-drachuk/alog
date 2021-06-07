#pragma once
#include <memory>
#include <vector>
#include <functional>
#include <alog/record.h>
#include <alog/tools.h>


namespace ALog {

class ISink
{
public:
    virtual ~ISink() = default;
    virtual void write(const Buffer& buffer, const Record& record) = 0;
    virtual void flush() { }
};

using ISinkPtr = std::shared_ptr<ISink>;

} // namespace ALog


namespace ALog {
namespace Sinks {

class Chain : public ISink
{
public:
    Chain() = default;
    Chain(const std::initializer_list<ISinkPtr>& filters);

    Chain& addSink(const ISinkPtr& sink) { m_sinks.push_back(sink); return *this; };
    void clear();
    inline bool empty() const { return m_sinks.empty(); };

    void write(const Buffer& buffer, const Record& record) override;
    void flush() override;

private:
    std::vector<ISinkPtr> m_sinks;
};



template<typename T>
class Functor : public ISink
{
public:
    Functor(const T& func): m_func(func) { }
    void write(const Buffer& buffer, const Record& record) override { m_func(buffer, record); }

private:
    T m_func;
};

using Functor2 = Functor<std::function<void(const Buffer& buffer, const Record&)>>;



class Null : public ISink
{
public:
    void write(const Buffer&, const Record&) override { }
};

} // namespace Sinks
} // namespace ALog
