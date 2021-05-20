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

bool isSeparatorSymbol(char c)
{
    static bool isSeparator[256] {false};
    static bool isReady {false};

    if (!isReady) {
        for (int i = 32; i <= 47; i++)
            isSeparator[i] = true;

        for (int i = 58; i <= 64; i++)
            isSeparator[i] = true;

        for (int i = 91; i <= 96; i++)
            isSeparator[i] = true;

        for (int i = 123; i <= 126; i++)
            isSeparator[i] = true;

        isSeparator[(int)':'] = false;

        isReady = true;
    }

    return isSeparator[(int)c];
}

} // namespace ALog
