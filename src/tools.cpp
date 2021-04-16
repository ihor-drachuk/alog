#include <alog/tools.h>

#include <atomic>
#include <cassert>

namespace ALog {

struct StaticCheck
{
    StaticCheck() { value = true; }
    ~StaticCheck() { value = false; }
    bool value;
};

namespace ThreadTools {

int currentThreadId()
{
    static StaticCheck sc;
    assert(sc.value && "currentThreadId: static data already deleted!");
    static std::atomic_int counter;
    thread_local int threadId = counter.fetch_add(1);
    return threadId;
}

const char*& accessThreadName()
{
    thread_local StaticCheck sc;
    assert(sc.value && "accessThreadName: static data already deleted!");
    thread_local const char* name = nullptr;
    return name;
}

void setCurrentThreadName(const char* title)
{
    accessThreadName() = title;
}

const char* currentThreadName()
{
    return accessThreadName();
}

} // namespace ThreadTools

} // namespace ALog
