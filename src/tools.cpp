#include <alog/tools.h>

#include <alog/record.h>

#include <atomic>
#include <cassert>

#ifdef ALOG_HAS_QT_LIBRARY
#include <QString>
#endif // ALOG_HAS_QT_LIBRARY

namespace ALog {
namespace Internal {

struct StaticCheck
{
    ALOG_NO_COPY_MOVE(StaticCheck);
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

        isSeparator[(unsigned char)':'] = false;

        isReady = true;
    }

    return isSeparator[(unsigned char)c];
}

#ifdef ALOG_HAS_QT_LIBRARY
void logJsonData(ALog::Record& record, const QString& jsonType, const QString& jsonContent)
{
    auto _flagsRestorer = record.backupFlags();

    auto jsonTypeU8 = jsonType.toUtf8();
    auto jsonContentU8 = jsonContent.toUtf8();

    assert(*jsonTypeU8.rbegin() != 0);

    record.appendMessage("{Json; ");
    record << ALog::Record::Flags::Internal_NoSeparators;
    record.appendMessage(jsonTypeU8.constData(), jsonTypeU8.size());
    record.appendMessage("; Content = \"");
    record.appendMessage(jsonContentU8.constData(), jsonContentU8.size());
    record.appendMessage("\"}");
}
#endif // ALOG_HAS_QT_LIBRARY

} // namespace Internal
} // namespace ALog
