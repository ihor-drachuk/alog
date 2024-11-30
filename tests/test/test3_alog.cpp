/* License:  MIT
 * Source:   https://github.com/ihor-drachuk/alog
 * Contact:  ihor-drachuk-libs@pm.me  */

#define QT_DISABLE_DEPRECATED_BEFORE 0x0A0000

#include <gtest/gtest.h>

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <numeric>
#include <regex>
#include <stdexcept>

#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <utility>

#ifdef ALOG_HAS_QT_LIBRARY
#include <QMetaType>
#include <QFuture>
#else
#define Q_NAMESPACE
#define Q_ENUM_NS(x)
#endif // ALOG_HAS_QT_LIBRARY

#include <alog/tools_filesystem.h>
#include <alog/all.h>

#include <alog/containers/all.h>

namespace {

constexpr std::chrono::hours operator""_d(unsigned long long d)
{
    return std::chrono::hours(d*24);
}

template<typename T,
         typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
bool floatsEquality(T a, T b)
{
    return ((a - b) <= std::numeric_limits<T>::epsilon());
}

std::string escapeRegex(const std::string& str)
{
    return std::regex_replace(str, std::regex(R"([-[\]{}()*+?.,\^$|#\s])"), R"(\$&)");
}

bool regexMatch(const std::string& str, const std::string& regex)
{
    return std::regex_match(str, std::regex(regex));
}

std::string stringReplace(const std::string& str, const std::string& from, const std::string& to)
{
    if (auto offset = str.find(from); offset != std::string::npos) {
        return std::string(str).replace(offset, from.size(), to);
    } else {
        return str;
    }
}

#ifdef ALOG_HAS_QT_LIBRARY
class QExceptionPtr : public QException
{
public:
    QExceptionPtr(const std::exception_ptr& eptr) : m_eptr(eptr) {} // NOLINT(bugprone-throw-keyword-missing)
    QExceptionPtr(std::exception_ptr&& eptr) : m_eptr(std::move(eptr)) {} // NOLINT(bugprone-throw-keyword-missing)
    QExceptionPtr(const QExceptionPtr&) = delete;
    QExceptionPtr(QExceptionPtr&&) noexcept = default;

    void raise() const override { if (m_eptr) std::rethrow_exception(m_eptr); }
    QException* clone() const override { return new QExceptionPtr(m_eptr); }

private:
    std::exception_ptr m_eptr;
};

template<typename T>
QFuture<T> createNotStartedFuture()
{
    return QFutureInterface<T>().future();
}

template<typename T>
QFuture<T> createRunningFuture()
{
    QFutureInterface<T> interface;
    interface.reportStarted();
    return interface.future();
}

template<typename T>
QFuture<T> createCanceledFuture()
{
    QFutureInterface<T> interface;
    interface.reportStarted();
    interface.reportCanceled();
    interface.reportFinished();
    return interface.future();
}

template<typename T>
QFuture<T> createFinishedFuture(const T& value)
{
    QFutureInterface<T> interface;
    interface.reportStarted();
    interface.reportResult(value);
    interface.reportFinished();
    return interface.future();
}

template<typename T>
QFuture<T> createExceptionFuture()
{
    QFutureInterface<T> interface;
    interface.reportStarted();
    try {
        throw std::runtime_error("Test");
    } catch (...) {
        interface.reportException(QExceptionPtr(std::current_exception()));
    }
    interface.reportFinished();

    return interface.future();
}
#endif // ALOG_HAS_QT_LIBRARY

} // namespace


TEST(ALog, test0)
{
    for (int i = 0; i < 100; i++) {
        ALog::DefaultLogger logger;
        (void)logger;
    }
}

TEST(ALog, test_simple_setup)
{
    SIMPLE_SETUP_ALOG;
    LOGMD << "Test!";
}


TEST(ALog, test_unicode)
{
    SIMPLE_SETUP_ALOG;
    LOGMD << "Привіт!";
}


TEST(ALog, test_lateMaster)
{
    DEFINE_ALOGGER_MODULE(ALogerTest);

    LOGD;

    ALog::DefaultLogger logger;
    logger.markReady();

    LOGD;

    ASSERT_EQ(true, true);
}

TEST(ALog, test_sink)
{
    ALog::Record record;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&record](const ALog::Buffer&, const ALog::Record& rec){ record = rec; });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    MARK_ALOGGER_READY;

    DEFINE_ALOGGER_MODULE(ALogerTest);
    LOGD;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    EXPECT_EQ(record.severity, ALog::Severity::Debug);
    EXPECT_EQ(strcmp(record.module, "ALogerTest"), 0);
}

TEST(ALog, test_sinkWithLateMaster)
{
    std::vector<ALog::Record> records;

    DEFINE_ALOGGER_MODULE(ALogerTest);
    LOGD;

    ASSERT_EQ(records.size(), 0);

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    MARK_ALOGGER_READY;

    ASSERT_EQ(records.size(), 0);

    LOGD;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(records.size(), 2);
    for (const auto& x : records) {
        EXPECT_EQ(x.severity, ALog::Severity::Debug);
        EXPECT_EQ(strcmp(x.module, "ALogerTest"), 0);
    }
}

TEST(ALog, test_syncMode)
{
    ALog::Record record;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&record](const ALog::Buffer&, const ALog::Record& rec){ record = rec; });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    MARK_ALOGGER_READY;

    DEFINE_ALOGGER_MODULE(ALogerTest);
    LOGD;
    EXPECT_EQ(record.severity, ALog::Severity::Debug);
    EXPECT_EQ(strcmp(record.module, "ALogerTest"), 0);
}

TEST(ALog, test_threadNames)
{
    const char* const thrName1 = "Thread 1";
    const char* const thrName2 = "Thread 2";
    const char* const thrName3 = "Thread 3";
    const char* const thrName4 = "Thread 4";
    const char* const thrName5 = "Thread 5";

    // Part 1
    std::thread t1([&](){
        ALog::I::ThreadTools::setCurrentThreadName(thrName1);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(thrName1, ALog::I::ThreadTools::currentThreadName());
    });

    std::thread t2([&](){
        ALog::I::ThreadTools::setCurrentThreadName(thrName2);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        EXPECT_EQ(thrName2, ALog::I::ThreadTools::currentThreadName());
    });

    std::thread t3([&](){
        ALog::I::ThreadTools::setCurrentThreadName(thrName3);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        EXPECT_EQ(thrName3, ALog::I::ThreadTools::currentThreadName());
    });

    t1.join();
    t2.join();
    t3.join();

    // Part 2
    std::vector<ALog::Record> records;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    MARK_ALOGGER_READY;

    DEFINE_ALOGGER_MODULE(ALogerTest);

    std::thread t4([&](){
        ALog::I::ThreadTools::setCurrentThreadName(thrName4);
        LOGD;
    });

    std::thread t5([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ALog::I::ThreadTools::setCurrentThreadName(thrName5);
        LOGD;
    });

    t4.join();
    t5.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(records.size(), 2);
    EXPECT_EQ(strcmp(records[0].threadTitle, thrName4), 0);
    EXPECT_EQ(strcmp(records[1].threadTitle, thrName5), 0);
    EXPECT_EQ(records[0].threadTitle, thrName4);
    EXPECT_EQ(records[1].threadTitle, thrName5);
}

#ifndef ALOG_CI_SKIP_SORT_TEST
TEST(ALog, test_sort)
{
    const auto strict = false;
    std::vector<ALog::Record> records;

    {
        DEFINE_MAIN_ALOGGER;
        auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
        ALOGGER_DIRECT->pipeline().sinks().set(sink2);
        ALOGGER_DIRECT->setMode( strict ? ALog::Logger::AsynchronousStrictSort : ALog::Logger::AsynchronousSort );
        MARK_ALOGGER_READY;

        bool ready = false;
        std::mutex mutex;
        std::condition_variable cv;

        std::vector<std::thread> threads;

        for (int i = 0; i < 200; i++) {
            threads.push_back(std::thread([&](){
                DEFINE_ALOGGER_MODULE(module);

                {
                    std::unique_lock<std::mutex> lock(mutex);
                    cv.wait(lock, [&](){ return ready; });
                }

                LOGI;
                LOGD;
                LOGW;
            }));
        }

        {
            std::unique_lock<std::mutex> lock(mutex);
            ready = true;
            cv.notify_all();
        }

        for (auto& x : threads)
            x.join();
    }

    int unsortedCount = 0;
    for (size_t i = 0; i < records.size() - 1; i++) {
        if (records[i].steadyTp > records[i+1].steadyTp)
            unsortedCount++;
    }

    if (strict) {
        ASSERT_EQ(unsortedCount, 0);
    } else {
        ASSERT_LT(unsortedCount, 20);
    }
}
#endif // ALOG_CI_SKIP_SORT_TEST

TEST(ALog, test_flush)
{
    for (int i = 0; i < 1000; i++) {
        ALog::Record record;

        ALog::DefaultLogger logger;
        auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&record](const ALog::Buffer&, const ALog::Record& rec){ record = rec; });
        logger->pipeline().sinks().set(sink2);
        logger.markReady();

        DEFINE_ALOGGER_MODULE(ALogerTest);
        LOGD;
        logger->flush();
        EXPECT_EQ(record.severity, ALog::Severity::Debug);
        EXPECT_EQ(strcmp(record.module, "ALogerTest"), 0);

        record.severity = ALog::Severity::Minimal;
        record.module = nullptr;
        LOGD << ALog::Record::Flags::Flush;
        EXPECT_EQ(record.severity, ALog::Severity::Debug);
        EXPECT_EQ(strcmp(record.module, "ALogerTest"), 0);

        record.severity = ALog::Severity::Minimal;
        record.module = nullptr;
        LOGD;
        ACCESS_ALOGGER_MODULE.flush();
        EXPECT_EQ(record.severity, ALog::Severity::Debug);
        EXPECT_EQ(strcmp(record.module, "ALogerTest"), 0);

        record.severity = ALog::Severity::Minimal;
        record.module = nullptr;
        LOGD << ALog::Record::Flags::FlushAndDrop;
        EXPECT_EQ(record.severity, ALog::Severity::Minimal);
        EXPECT_EQ(record.module, nullptr);
    }
}

TEST(ALog, test_operatorPutStream)
{
    std::vector<ALog::Record> records;

    ALog::DefaultLogger logger;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
    logger->pipeline().sinks().set(sink2);
    logger.markReady();

    DEFINE_ALOGGER_MODULE(ALogerTest);

    // Write integer
    LOGD << (uint8_t)1;
    LOGD << (int8_t)1;
    LOGD << (uint16_t)1;
    LOGD << (int16_t)1;
    LOGD << (uint32_t)1;
    LOGD << (int32_t)1;
    LOGD << (uint64_t)1;
    LOGD << (int64_t)1 << ALOG_FL_FLUSH;

    for (const auto& x : records)
        EXPECT_EQ(std::string(x.getMessage()), "1");

    // Min, Max
    LOGD << std::numeric_limits<uint8_t>::max() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint8_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint8_t>::min() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint8_t>::min()).data()), 0);

    LOGD << std::numeric_limits<uint16_t>::max() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint16_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint16_t>::min() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint16_t>::min()).data()), 0);

    LOGD << std::numeric_limits<uint32_t>::max() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint32_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint32_t>::min() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint32_t>::min()).data()), 0);

    LOGD << std::numeric_limits<uint64_t>::max() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint64_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint64_t>::min() << ALOG_FL_FLUSH;
    EXPECT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint64_t>::min()).data()), 0);

    records.clear();
    LOGD << (float)1.1;
    LOGD << (double)1.1;
    LOGD << (long double)1.1 << ALOG_FL_FLUSH;
    for (const auto& x : records)
        EXPECT_TRUE(floatsEquality( std::stof(std::string(x.getMessage(), x.getMessage() + x.getMessageLen()).data()), (float)1.1 ));

    records.clear();
    LOGD << "Test";
    LOGD << L"Test";
    LOGD << NO_AUTO_QUOTES << std::string("Test");
    LOGD << NO_AUTO_QUOTES << std::wstring(L"Test");
    logger->flush();
    for (const auto& x : records)
        EXPECT_EQ(std::string(x.getMessage()), "Test");

    records.clear();
    LOGD << QUOTE_LITERALS << "Test";
    LOGD << QUOTE_LITERALS << L"Test";
    logger->flush();
    for (const auto& x : records)
        EXPECT_EQ(std::string(x.getMessage()), "\"Test\"");

    records.clear();
    LOGD << std::string("Test");
    LOGD << std::wstring(L"Test");
    logger->flush();
#ifdef ALOG_ENABLE_DEF_AUTO_QUOTES
    for (const auto& x : records)
        EXPECT_EQ(std::string(x.getMessage()), "\"Test\"");
#else
    for (const auto& x : records)
        EXPECT_EQ(std::string(x.getMessage()), "Test");
#endif
}

TEST(ALog, test_defaultFormatter)
{
    ALog::Formatters::Default formatter;

    // #1
    auto record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(14);
    auto str1 = formatter.format(record);

    std::string recordedLog;
    recordedLog.resize(str1.size());
    memcpy(recordedLog.data(), str1.data(), str1.size());

    const auto reference1 = stringReplace(
                                escapeRegex("[    0.014] T#0  [Info    ] [::TestBody:NNNN]  Test"),
                                "NNNN", R"(\d+)");
    EXPECT_TRUE(regexMatch(recordedLog, reference1));

    // #2
    record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(14);
    record.threadTitle = "Worker";
    str1 = formatter.format(record);

    recordedLog.resize(str1.size());
    memcpy(recordedLog.data(), str1.data(), str1.size());

    const auto reference2 = stringReplace(
                                escapeRegex("[    0.014] T#0  (Worker) [Info    ] [::TestBody:NNNN]  Test"),
                                "NNNN", R"(\d+)");
    EXPECT_TRUE(regexMatch(recordedLog, reference2));

    // #3
    record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(14);
    record.module = "Module";
    str1 = formatter.format(record);

    recordedLog.resize(str1.size());
    memcpy(recordedLog.data(), str1.data(), str1.size());

    const auto reference3 = stringReplace(
                                escapeRegex("[    0.014] T#0  [Info    ] [Module               ] [::TestBody:NNNN]  Test"),
                                "NNNN", R"(\d+)");
    EXPECT_TRUE(regexMatch(recordedLog, reference3));

    // #4
    record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(13 * 1000 + 14);
    record.threadTitle = "Worker";
    record.module = "Module";
    str1 = formatter.format(record);

    recordedLog.resize(str1.size());
    memcpy(recordedLog.data(), str1.data(), str1.size());

    const auto reference4 = stringReplace(
                                escapeRegex("[   13.014] T#0  (Worker) [Info    ] [Module               ] [::TestBody:NNNN]  Test"),
                                "NNNN", R"(\d+)");
    EXPECT_TRUE(regexMatch(recordedLog, reference4));

    // #5
    record.flagsOn(ALog::Record::Flags::Abort);
    str1 = formatter.format(record);

    recordedLog.resize(str1.size());
    memcpy(recordedLog.data(), str1.data(), str1.size());
    EXPECT_NE(strstr(recordedLog.data(), "test3_alog.cpp"), nullptr);
}

TEST(ALog, test_defaultFormatterLate)
{
    DEFINE_ALOGGER_MODULE(ALogerTest);
    ASSERT_THROW(LOGD << "1" << THROW, std::runtime_error);
    ALog::DefaultLogger logger;
    ALog::Record record[2];
    int recordIdx = 0;
    logger->pipeline().sinks().set(std::make_shared<ALog::Sinks::Functor2>([&](const ALog::Buffer&, const ALog::Record& rec){
        record[recordIdx++] = rec;
    }));
    logger.markReady();
    LOGD << "Just trigger" << FLUSH;

    ASSERT_EQ(recordIdx, 2);

    ALog::Formatters::Default formatter;
    auto str1 = formatter.format(record[0]);
    std::string str2;

    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    EXPECT_NE(strstr(str2.data(), "test3_alog.cpp"), nullptr);

    str1 = formatter.format(record[1]);
    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    EXPECT_EQ(strstr(str2.data(), "test3_alog.cpp"), nullptr);
    EXPECT_NE(strstr(str2.data(), "Just trigger"), nullptr);
}

TEST(ALog, test_hex)
{
    const uint8_t buffer[] = {1, 2, 3, 4, 5, 6};
    char buffer2[1024];

    auto record = _ALOG_RECORD(ALog::Severity::Info) << BUFFER(buffer, sizeof(buffer));
    snprintf(buffer2, sizeof(buffer2), "{Buffer; Size: 6, Ptr = 0x%p, Data = 0x010203040506}", buffer);
    EXPECT_STREQ(record.getMessage(), buffer2);

    record = _ALOG_RECORD(ALog::Severity::Info) << BUFFER(buffer, 0);
    snprintf(buffer2, sizeof(buffer2), "{Buffer; Size: 0, Ptr = 0x%p. No data}", buffer);
    EXPECT_STREQ(record.getMessage(), buffer2);
}

TEST(ALog, test_flags)
{
    auto record = _ALOG_RECORD(ALog::Severity::Info);
    record << ABORT;
    ASSERT_TRUE(record.hasFlags(ALog::Record::Flags::AbortSync));
    record -= ABORT;
    ASSERT_FALSE(record.hasFlagsAny(ALog::Record::Flags::AbortSync));
}

TEST(ALog, test_separators)
{
    std::vector<ALog::Record> records;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    MARK_ALOGGER_READY;

    LOGMD << SEP(" ")       << "String-1" << 1 << "String-2";
    LOGMD.seps(" ")         << "String-1" << 1 << "String-2";
    LOGMD <<                   "String-1" << 1 << "String-2";
    LOGMD << NO_SEPARATORS  << "String-1" << 1 << "String-2";
    LOGMD.no_seps()         << "String-1" << 1 << "String-2";
    LOGMD << SEP(", ")      << "String-1" << 1 << "String-2";
    LOGMD << SEP(" ") << OSEP("_") << "String-1" << 1 << "String-2";
    LOGMD << NO_SEPARATORS << OSEP("_") << "String-1" << 1 << "String-2";
    LOGMD << NO_SEPARATORS << "String-1" << 1 << OSEP("_") << "String-2";

    MainALogger_0->flush();

    ASSERT_EQ(records.size(), 9);
    EXPECT_STREQ(records[0].message.getString(), "String-1 1 String-2");
    EXPECT_STREQ(records[1].message.getString(), "String-1 1 String-2");
#ifdef ALOG_ENABLE_DEF_SEPARATORS
    EXPECT_STREQ(records[2].message.getString(), "String-1 1 String-2");
#else
    EXPECT_STREQ(records[2].message.getString(), "String-11String-2");
#endif
    EXPECT_STREQ(records[3].message.getString(), "String-11String-2");
    EXPECT_STREQ(records[4].message.getString(), "String-11String-2");
    EXPECT_STREQ(records[5].message.getString(), "String-1, 1, String-2");
    EXPECT_STREQ(records[6].message.getString(), "String-1_1 String-2");
    EXPECT_STREQ(records[7].message.getString(), "String-1_1String-2");
    EXPECT_STREQ(records[8].message.getString(), "String-11_String-2");
}

TEST(ALog, test_filters)
{
    std::vector<ALog::Record> records;

    auto filters = ALog::Filters::Chain::create({
        // Always pass warnings
        std::make_shared<ALog::Filters::Severity>(ALog::Severity::Warning, ALog::IFilter::PassOrUndefined),

        // Example: Hide DeviceFeaturesFactory
        std::make_shared<ALog::Filters::Module>("module1", false, ALog::IFilter::RejectOrUndefined),

        // Pass everything from FirebaseIntegration
        std::make_shared<ALog::Filters::Module>("module2", true, ALog::IFilter::PassOrUndefined),

        // Explicit default decision - reject
        std::make_shared<ALog::Filters::Always>(false)
    });

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    ALOGGER_DIRECT->pipeline().filters().set(filters);
    ALOGGER_DIRECT->setMode(ALog::Logger::LoggerMode::Synchronous);
    MARK_ALOGGER_READY;


    {
        DEFINE_ALOGGER_MODULE(module1);
        LOGD << "1";
        LOGW << "2";
        LOGE << "3";
    }

    {
        DEFINE_ALOGGER_MODULE(module2);
        LOGD << "4";
        LOGW << "5";
        LOGE << "6";
    }

    {
        DEFINE_ALOGGER_MODULE(module3);
        LOGD << "7";
        LOGW << "8";
        LOGE << "9";
    }

    std::set<std::string> expected {"2", "3", "5", "6", "8", "9",  "4"};
    std::set<std::string> unexpected {"1", "7"};
    std::set<std::string> items;

    for (const auto& x : records)
        items.insert(x.getMessage());

    for (const auto& x : expected) {
        EXPECT_GE(items.count(x), 1);
    }

    for (const auto& x : unexpected) {
        EXPECT_EQ(items.count(x), 0);
    }
}

TEST(ALog, test_separators_advanced)
{
#if 0
    std::vector<ALog::Record> records;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    MARK_ALOGGER_READY;

    LOGMD << "Literal string" << "SSS" << "(SSS)";
    LOGMD << L"Literal string (wide)";
    LOGMD << std::string("String");
    LOGMD << std::wstring(L"String (wide)");
    LOGMD << std::pair<int, std::string>(11, "Hi") << "SSS";
    LOGMD << std::pair<std::string, int>("Hi", 11) << "SSS";
    LOGMD << NO_SEPARATORS << std::pair<int, std::string>(11, "Hi") << "SSS-no-sep";
    LOGMD << std::pair<int, std::pair<int, std::string>>(11, {12, "Hi"}) ;

    LOGMD << std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string>>({"1", "2"}, {"3", "4"});

    LOGMD << std::vector<int>{1, 2, 3, 4} << "SSS";
    LOGMD << std::list<int>{1, 2, 3, 4};
    LOGMD << std::set<int>{1, 2, 3, 4};
    LOGMD << std::unordered_set<int>{1, 2, 3, 4};
    LOGMD << std::map<int, std::string>{{1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    LOGMD << std::unordered_map<int, std::string>{{1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}} << "SSS";

    LOGMD << std::vector<std::pair<int, const char*>>{{1, "1"}, {2, "2"}, {3, "3"}};

    LOGMD << std::vector<std::vector<std::pair<int, const char*>>> {
        {{1, "1"}, {2, "2"}, {3, "3"}},
        {{4, "4"}, {5, "5"}},
    };

    MainALogger_0->flush();

    ASSERT_EQ(records.size(), 17);
    EXPECT_STREQ(records[0].message.getString(), "Literal string SSS(SSS)");
    EXPECT_STREQ(records[1].message.getString(), "Literal string (wide)");
    EXPECT_STREQ(records[2].message.getString(), "\"String\"");
    EXPECT_STREQ(records[3].message.getString(), "\"String (wide)\"");
    EXPECT_STREQ(records[4].message.getString(), "(11, \"Hi\") SSS");
    EXPECT_STREQ(records[5].message.getString(), "(\"Hi\", 11) SSS");
    EXPECT_STREQ(records[6].message.getString(), "(11, \"Hi\")SSS-no-sep");
    EXPECT_STREQ(records[7].message.getString(), "(11, (12, \"Hi\"))");
    EXPECT_STREQ(records[8].message.getString(), "((\"1\", \"2\"), (\"3\", \"4\"))");
    EXPECT_STREQ(records[9].message.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4} SSS");
    EXPECT_STREQ(records[10].message.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4}");
    EXPECT_STREQ(records[11].message.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4}");
    //EXPECT_STREQ(records[12].str.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4}");
    EXPECT_STREQ(records[13].message.getString(), "{Container; Size: 4; Data = (1, \"1\"), (2, \"2\"), (3, \"3\"), (4, \"4\")}");
    //EXPECT_STREQ(records[14].str.getString(), "{Container; Size: 4; Data = (1, \"1\"), (2, \"2\"), (3, \"3\"), (4, \"4\")} SSS");
    EXPECT_STREQ(records[15].message.getString(), "{Container; Size: 3; Data = (1, \"1\"), (2, \"2\"), (3, \"3\")}");
    EXPECT_STREQ(records[16].message.getString(), "{Container; Size: 2; Data = {Container; Size: 3; Data = (1, \"1\"), (2, \"2\"), (3, \"3\")}, {Container; Size: 2; Data = (4, \"4\"), (5, \"5\")}}");
#endif
}

// -- TEST(ALog, test_enums) context --

namespace TestNamespace {
Q_NAMESPACE
enum class TestEnum {
    TestValue1 = 1,
    TestValue2
};
Q_ENUM_NS(TestEnum)

enum class RawTestEnum {
    TestValue1 = 1,
    TestValue2
};
} // namespace TestNamespace

#ifndef ALOG_HAS_QT_LIBRARY
#undef Q_NAMESPACE
#undef Q_ENUM_NS
#endif // !ALOG_HAS_QT_LIBRARY

TEST(ALog, test_enums)
{
    std::vector<ALog::Record> records;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->pipeline().sinks().set(sink2);
    MARK_ALOGGER_READY;

    LOGMD << TestNamespace::TestEnum::TestValue2;
    LOGMD << static_cast<TestNamespace::TestEnum>(10);
    LOGMD << TestNamespace::RawTestEnum::TestValue1;

    ALOGGER_DIRECT->flush();

    ASSERT_EQ(records.size(), 3);
#ifdef ALOG_HAS_QT_LIBRARY
    EXPECT_STREQ(records[0].message.getString(), "TestEnum(2, TestValue2)");
    EXPECT_EQ   (records[0].severity,            ALog::Severity::Debug);
    EXPECT_STREQ(records[1].message.getString(), "TestEnum(10, out-of-range)");
    EXPECT_EQ   (records[1].severity,            ALog::Severity::Warning);
    EXPECT_STREQ(records[2].message.getString(), "1");
    EXPECT_EQ   (records[2].severity,            ALog::Severity::Debug);
#else
    EXPECT_STREQ(records[0].message.getString(), "2");
    EXPECT_STREQ(records[1].message.getString(), "10");
    EXPECT_STREQ(records[2].message.getString(), "1");
#endif // ALOG_HAS_QT_LIBRARY
}

// -- --


TEST(ALog, test_sink_file_rotated)
{
    // Tools
    using Clock = std::chrono::system_clock;

    struct FileInfo {
        std::string name;
        size_t size;
        std::string content;

        bool operator< (const FileInfo& rhs) const {
            if (name.size() < rhs.name.size())
                return true;

            return name < rhs.name;
        }
    };

    auto fetchFiles = [](const std::string& path){
        std::vector<FileInfo> result;

        for (const auto& x : std::filesystem::directory_iterator(path)) {
            FileInfo info;
            info.name = x.path().filename().string();
            info.size = fsSize(x);

            FILE* f = fopen(x.path().string().c_str(), "r");
            assert(f);
            info.content.resize(info.size);
            const auto rd = fread(info.content.data(), 1, info.content.size(), f);
            assert(rd == info.size);
            fclose(f);

            result.push_back(std::move(info));
        }

        std::sort(result.begin(), result.end());

        return result;
    };

    auto createFileRotated = [](const std::string& fileName){
        return std::make_shared<ALog::Sinks::FileRotated>(fileName, false, 12, std::optional<size_t>{}, 4);
    };

    // Prepare sandbox
    const std::string sandboxFolder = "alog-test-" + std::to_string(Clock::now().time_since_epoch().count());
    const auto sandboxPath = std::filesystem::path(sandboxFolder + "/fn").remove_filename();
    const bool folderAlreadyExists = std::filesystem::exists(sandboxPath);
    ASSERT_FALSE(folderAlreadyExists);

    std::filesystem::create_directory(sandboxPath); // throws
    const auto _undo_create_directory = ALog::Internal::CreateFinally([sandboxPath](){ std::filesystem::remove_all(sandboxPath); });

    // Initial test
    {
        {
            DEFINE_MAIN_ALOGGER;
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            ALOGGER_DIRECT->pipeline().sinks().set(createFileRotated(sandboxFolder + "/application.log"));
            ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
            ALOGGER_DIRECT.markReady();
            DEFINE_ALOGGER_MODULE(ALogTest);
            LOGI << "Some";
        }

        {
            DEFINE_MAIN_ALOGGER;
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            ALOGGER_DIRECT->pipeline().sinks().set(createFileRotated(sandboxFolder + "/application.log"));
            ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
            ALOGGER_DIRECT.markReady();
            DEFINE_ALOGGER_MODULE(ALogTest);
            LOGI << " log";
        }

        auto files = fetchFiles(sandboxFolder);

        ASSERT_EQ(files.size(), 1);
        EXPECT_EQ(files[0].name, "application.log");
        EXPECT_EQ(files[0].content, "Some\n log\n");
    }

    // Size rotation test
    {
        {
            DEFINE_MAIN_ALOGGER;
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            ALOGGER_DIRECT->pipeline().sinks().set(createFileRotated(sandboxFolder + "/application.log"));
            ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
            ALOGGER_DIRECT.markReady();
            DEFINE_ALOGGER_MODULE(ALogTest);
            LOGI << "Some log2";
        }

        auto files = fetchFiles(sandboxFolder);

        ASSERT_EQ(files.size(), 2);
        EXPECT_EQ(files[1].name, "application.log.2");
        EXPECT_EQ(files[1].content, "Some\n log\n");

        EXPECT_EQ(files[0].name, "application.log");
        EXPECT_EQ(files[0].content, "Some log2\n");
    }

    // Size rotation test #2 + limit test
    {
        {
            DEFINE_MAIN_ALOGGER;
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            ALOGGER_DIRECT->pipeline().sinks().set(createFileRotated(sandboxFolder + "/application.log"));
            ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
            ALOGGER_DIRECT.markReady();
            DEFINE_ALOGGER_MODULE(ALogTest);
            LOGI << "Some log3";
        }

        auto files = fetchFiles(sandboxFolder);

        ASSERT_EQ(files.size(), 3);
        EXPECT_EQ(files[2].name, "application.log.3");
        EXPECT_EQ(files[2].content, "Some\n log\n");

        EXPECT_EQ(files[1].name, "application.log.2");
        EXPECT_EQ(files[1].content, "Some log2\n");

        EXPECT_EQ(files[0].name, "application.log");
        EXPECT_EQ(files[0].content, "Some log3\n");
    }

    {
        {
            DEFINE_MAIN_ALOGGER;
            ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
            ALOGGER_DIRECT->pipeline().sinks().set(createFileRotated(sandboxFolder + "/application.log"));
            ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
            ALOGGER_DIRECT.markReady();
            DEFINE_ALOGGER_MODULE(ALogTest);
            LOGI << "Some log4";
            LOGI << "Some log5";
        }

        auto files = fetchFiles(sandboxFolder);

        ASSERT_EQ(files.size(), 4);
        EXPECT_EQ(files[3].name, "application.log.4");
        EXPECT_EQ(files[3].content, "Some log2\n");

        EXPECT_EQ(files[2].name, "application.log.3");
        EXPECT_EQ(files[2].content, "Some log3\n");

        EXPECT_EQ(files[1].name, "application.log.2");
        EXPECT_EQ(files[1].content, "Some log4\n");

        EXPECT_EQ(files[0].name, "application.log");
        EXPECT_EQ(files[0].content, "Some log5\n");
    }

    // Redundant files count limit test (on start)
    {
        for (int i = 5; i < 10; i++) {
            FILE* f = fopen((sandboxFolder + "/application.log." + std::to_string(i)).c_str(), "w");
            fclose(f);
        }

        {
            DEFINE_MAIN_ALOGGER;
            ALOGGER_DIRECT->setMode(ALog::Logger::Asynchronous);
            ALOGGER_DIRECT->pipeline().sinks().set(createFileRotated(sandboxFolder + "/application.log"));
            ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
            ALOGGER_DIRECT.markReady();
            DEFINE_ALOGGER_MODULE(ALogTest);
        }

        auto files = fetchFiles(sandboxFolder);

        ASSERT_EQ(files.size(), 4);
        EXPECT_EQ(files[3].name, "application.log.4");
        EXPECT_EQ(files[3].content, "Some log2\n");

        EXPECT_EQ(files[2].name, "application.log.3");
        EXPECT_EQ(files[2].content, "Some log3\n");

        EXPECT_EQ(files[1].name, "application.log.2");
        EXPECT_EQ(files[1].content, "Some log4\n");

        EXPECT_EQ(files[0].name, "application.log");
        EXPECT_EQ(files[0].content, "Some log5\n");
    }
}

TEST(ALog, test_optional)
{
    std::vector<ALog::Record> records;
    auto sink = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });

    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    ALOGGER_DIRECT->pipeline().sinks().set(sink);
    ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
    ALOGGER_DIRECT.markReady();
    DEFINE_ALOGGER_MODULE(ALogTest);

    LOGI << std::optional<int>();
    LOGI << std::optional<int>(10);

    ASSERT_EQ(records.size(), 2);
    EXPECT_STREQ(records[0].getMessage(), "std::optional()");
    EXPECT_STREQ(records[1].getMessage(), "std::optional(10)");
}

TEST(ALog, test_variant)
{
    std::vector<ALog::Record> records;
    auto sink = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });

    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    ALOGGER_DIRECT->pipeline().sinks().set(sink);
    ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
    ALOGGER_DIRECT.markReady();
    DEFINE_ALOGGER_MODULE(ALogTest);

    std::variant<std::monostate, int, std::string> value;

    LOGI << value;

    value = 10;
    LOGI << value;

    value = "Hello";
    LOGI << value;

    ASSERT_EQ(records.size(), 3);
    EXPECT_STREQ(records[0].getMessage(), R"(std::variant(std::monostate()))");
    EXPECT_STREQ(records[1].getMessage(), R"(std::variant(10))");
#ifdef ALOG_ENABLE_DEF_AUTO_QUOTES
    EXPECT_STREQ(records[2].getMessage(), R"(std::variant("Hello"))");
#else
    EXPECT_STREQ(records[2].getMessage(), R"(std::variant(Hello))");
#endif // ALOG_ENABLE_DEF_AUTO_QUOTES
}

TEST(ALog, test_chrono_duration)
{
    using namespace std::literals::chrono_literals;

    std::vector<ALog::Record> records;
    auto sink = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });

    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    ALOGGER_DIRECT->pipeline().sinks().set(sink);
    ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
    ALOGGER_DIRECT.markReady();
    DEFINE_ALOGGER_MODULE(ALogTest);

    LOGI << (2_d + 15h);
    LOGI << 8h + 13min + 20s;
    LOGI << 10min + 20s + 113ms;
    LOGI << 20s + 113ms;
    LOGI << 25ms;
    LOGI << 5ms + 62us;

    ASSERT_EQ(records.size(), 6);
    EXPECT_STREQ(records[0].getMessage(), R"(std::chrono::duration(2d 15h))");
    EXPECT_STREQ(records[1].getMessage(), R"(std::chrono::duration(8h:13m:20s))");
    EXPECT_STREQ(records[2].getMessage(), R"(std::chrono::duration(10m:20s))");
    EXPECT_STREQ(records[3].getMessage(), R"(std::chrono::duration(20.113 sec))");
    EXPECT_STREQ(records[4].getMessage(), R"(std::chrono::duration(25 ms))");
    EXPECT_STREQ(records[5].getMessage(), R"(std::chrono::duration(5.062 ms))");
}


TEST(ALog, test_smart_pointers)
{
    using namespace std::literals::chrono_literals;

    std::vector<ALog::Record> records;
    auto sink = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });

    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    ALOGGER_DIRECT->pipeline().sinks().set(sink);
    ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
    ALOGGER_DIRECT.markReady();
    DEFINE_ALOGGER_MODULE(ALogTest);

    LOGI << std::shared_ptr<int>();
    LOGI << std::make_shared<int>(10);
    LOGI << std::unique_ptr<std::chrono::milliseconds>();
    LOGI << std::make_unique<std::chrono::milliseconds>(25ms);

    ASSERT_EQ(records.size(), 4);
    EXPECT_STREQ(records[0].getMessage(), R"(std::shared_ptr(nullptr))");
    EXPECT_STREQ(records[1].getMessage(), R"(std::shared_ptr(10))");
    EXPECT_STREQ(records[2].getMessage(), R"(std::unique_ptr(nullptr))");
    EXPECT_STREQ(records[3].getMessage(), R"(std::unique_ptr(std::chrono::duration(25 ms)))");
}

#ifdef ALOG_HAS_QT_LIBRARY
// Test futures print: not started, running, canceled, finished, exception
TEST(ALog, test_QFuture)
{
    using namespace std::literals::chrono_literals;
    using Type = std::chrono::milliseconds;

    std::vector<ALog::Record> records;
    auto sink = std::make_shared<ALog::Sinks::Functor2>([&records](const ALog::Buffer&, const ALog::Record& rec){ records.push_back(rec); });

    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    ALOGGER_DIRECT->pipeline().sinks().set(sink);
    ALOGGER_DIRECT->pipeline().formatter() = std::make_shared<ALog::Formatters::Minimal>();
    ALOGGER_DIRECT.markReady();
    DEFINE_ALOGGER_MODULE(ALogTest);

    LOGI << createNotStartedFuture<Type>();
    LOGI << createRunningFuture<Type>();
    LOGI << createCanceledFuture<Type>();
    LOGI << createFinishedFuture<Type>(25ms);
    LOGI << createExceptionFuture<Type>();

    ASSERT_EQ(records.size(), 5);
    EXPECT_STREQ(records[0].getMessage(), R"(QFuture(not started))");
    EXPECT_STREQ(records[1].getMessage(), R"(QFuture(running))");
    EXPECT_STREQ(records[2].getMessage(), R"(QFuture(canceled))");
    EXPECT_STREQ(records[3].getMessage(), R"(QFuture(finished: std::chrono::duration(25 ms)))");
    EXPECT_STREQ(records[4].getMessage(), R"(QFuture(exception: "Test"))");
}
#endif // ALOG_HAS_QT_LIBRARY

#ifdef ALOG_HAS_QT_LIBRARY
#include "test3_alog.moc"
#endif
