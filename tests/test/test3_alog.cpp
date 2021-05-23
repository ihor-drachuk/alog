#define QT_DISABLE_DEPRECATED_BEFORE 0x0A0000

#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include <numeric>

#include <string>
#include <vector>
#include <list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <utility>

#include <gtest/gtest.h>
#include "alog/logger.h"
#include "alog/formatter.h"
#include "alog/exceptions.h"

template<typename T,
         typename std::enable_if<std::is_floating_point<T>::value, int>::type = 0>
bool floatsEquality(T a, T b)
{
    return ((a - b) <= std::numeric_limits<T>::epsilon());
}

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
    LOGMD << "Привет!";
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
    auto sink2 = std::make_shared<ALog::SinkFunctor2>([&record](const ALog::Record& rec){ record = rec; });
    ALOGGER_DIRECT->setSink(sink2);
    MARK_ALOGGER_READY;

    DEFINE_ALOGGER_MODULE(ALogerTest);
    LOGD;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(record.severity, ALog::Severity::Debug);
    ASSERT_EQ(strcmp(record.module, "ALogerTest"), 0);
}

TEST(ALog, test_sinkWithLateMaster)
{
    std::vector<ALog::Record> records;

    DEFINE_ALOGGER_MODULE(ALogerTest);
    LOGD;

    ASSERT_EQ(records.size(), 0);

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::SinkFunctor2>([&records](const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->setSink(sink2);
    MARK_ALOGGER_READY;

    ASSERT_EQ(records.size(), 0);

    LOGD;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    ASSERT_EQ(records.size(), 2);
    for (const auto& x : records) {
        ASSERT_EQ(x.severity, ALog::Severity::Debug);
        ASSERT_EQ(strcmp(x.module, "ALogerTest"), 0);
    }
}

TEST(ALog, test_syncMode)
{
    ALog::Record record;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::SinkFunctor2>([&record](const ALog::Record& rec){ record = rec; });
    ALOGGER_DIRECT->setSink(sink2);
    ALOGGER_DIRECT->setMode(ALog::Logger::Synchronous);
    MARK_ALOGGER_READY;

    DEFINE_ALOGGER_MODULE(ALogerTest);
    LOGD;
    ASSERT_EQ(record.severity, ALog::Severity::Debug);
    ASSERT_EQ(strcmp(record.module, "ALogerTest"), 0);
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
        ALog::ThreadTools::setCurrentThreadName(thrName1);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ASSERT_EQ(thrName1, ALog::ThreadTools::currentThreadName());
    });

    std::thread t2([&](){
        ALog::ThreadTools::setCurrentThreadName(thrName2);
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        ASSERT_EQ(thrName2, ALog::ThreadTools::currentThreadName());
    });

    std::thread t3([&](){
        ALog::ThreadTools::setCurrentThreadName(thrName3);
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ASSERT_EQ(thrName3, ALog::ThreadTools::currentThreadName());
    });

    t1.join();
    t2.join();
    t3.join();

    // Part 2
    std::vector<ALog::Record> records;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::SinkFunctor2>([&records](const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->setSink(sink2);
    MARK_ALOGGER_READY;

    DEFINE_ALOGGER_MODULE(ALogerTest);

    std::thread t4([&](){
        ALog::ThreadTools::setCurrentThreadName(thrName4);
        LOGD;
    });

    std::thread t5([&](){
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
        ALog::ThreadTools::setCurrentThreadName(thrName5);
        LOGD;
    });

    t4.join();
    t5.join();

    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    ASSERT_EQ(records.size(), 2);
    ASSERT_EQ(strcmp(records[0].threadTitle, thrName4), 0);
    ASSERT_EQ(strcmp(records[1].threadTitle, thrName5), 0);
    ASSERT_EQ(records[0].threadTitle, thrName4);
    ASSERT_EQ(records[1].threadTitle, thrName5);
}

#ifndef ALOG_CI_SKIP_SORT_TEST
TEST(ALog, test_sort)
{
    const auto strict = false;
    std::vector<ALog::Record> records;

    {
        DEFINE_MAIN_ALOGGER;
        auto sink2 = std::make_shared<ALog::SinkFunctor2>([&records](const ALog::Record& rec){ records.push_back(rec); });
        ALOGGER_DIRECT->setSink(sink2);
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
        auto sink2 = std::make_shared<ALog::SinkFunctor2>([&record](const ALog::Record& rec){ record = rec; });
        logger->setSink(sink2);
        logger.markReady();

        DEFINE_ALOGGER_MODULE(ALogerTest);
        LOGD;
        logger->flush();
        ASSERT_EQ(record.severity, ALog::Severity::Debug);
        ASSERT_EQ(strcmp(record.module, "ALogerTest"), 0);

        record.severity = ALog::Severity::Minimal;
        record.module = nullptr;
        LOGD << ALog::Record::Flags::Flush;
        ASSERT_EQ(record.severity, ALog::Severity::Debug);
        ASSERT_EQ(strcmp(record.module, "ALogerTest"), 0);

        record.severity = ALog::Severity::Minimal;
        record.module = nullptr;
        LOGD;
        ACCESS_ALOGGER_MODULE.flush();
        ASSERT_EQ(record.severity, ALog::Severity::Debug);
        ASSERT_EQ(strcmp(record.module, "ALogerTest"), 0);

        record.severity = ALog::Severity::Minimal;
        record.module = nullptr;
        LOGD << ALog::Record::Flags::FlushAndDrop;
        ASSERT_EQ(record.severity, ALog::Severity::Minimal);
        ASSERT_EQ(record.module, nullptr);
    }
}

TEST(ALog, test_operatorPutStream)
{
    std::vector<ALog::Record> records;

    ALog::DefaultLogger logger;
    auto sink2 = std::make_shared<ALog::SinkFunctor2>([&records](const ALog::Record& rec){ records.push_back(rec); });
    logger->setSink(sink2);
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
        ASSERT_EQ(std::string(x.getMessage()), "1");

    // Min, Max
    LOGD << std::numeric_limits<uint8_t>::max() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint8_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint8_t>::min() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint8_t>::min()).data()), 0);

    LOGD << std::numeric_limits<uint16_t>::max() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint16_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint16_t>::min() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint16_t>::min()).data()), 0);

    LOGD << std::numeric_limits<uint32_t>::max() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint32_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint32_t>::min() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint32_t>::min()).data()), 0);

    LOGD << std::numeric_limits<uint64_t>::max() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint64_t>::max()).data()), 0);

    LOGD << std::numeric_limits<uint64_t>::min() << ALOG_FL_FLUSH;
    ASSERT_EQ( strcmp(records.back().getMessage(), std::to_string(std::numeric_limits<uint64_t>::min()).data()), 0);

    records.clear();
    LOGD << (float)1.1;
    LOGD << (double)1.1;
    LOGD << (long double)1.1 << ALOG_FL_FLUSH;
    for (const auto& x : records)
        ASSERT_TRUE(floatsEquality( std::stof(std::string(x.getMessage(), x.getMessage() + x.getMessageLen()).data()), (float)1.1 ));

    records.clear();
    LOGD << "Test";
    LOGD << L"Test";
    LOGD << NO_AUTO_QUOTES << std::string("Test");
    LOGD << NO_AUTO_QUOTES << std::wstring(L"Test");
    logger->flush();
    for (const auto& x : records)
        ASSERT_EQ(std::string(x.getMessage()), "Test");

    records.clear();
    LOGD << QUOTE_LITERALS << "Test";
    LOGD << QUOTE_LITERALS << L"Test";
    logger->flush();
    for (const auto& x : records)
        ASSERT_EQ(std::string(x.getMessage()), "\"Test\"");

    records.clear();
    LOGD << std::string("Test");
    LOGD << std::wstring(L"Test");
    logger->flush();
#ifdef ALOG_ENABLE_DEF_AUTO_QUOTES
    for (const auto& x : records)
        ASSERT_EQ(std::string(x.getMessage()), "\"Test\"");
#else
    for (const auto& x : records)
        ASSERT_EQ(std::string(x.getMessage()), "Test");
#endif
}

TEST(ALog, test_defaultFormatter)
{
    ALog::DefaultFormatter formatter;

    // #1
    auto record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(14);
    auto str1 = formatter.format(record);

    std::string str2;
    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());

    ASSERT_EQ(str2, "[    0.014] T#0  [Info    ] [::TestBody:372]  Test");

    // #2
    record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(14);
    record.threadTitle = "Worker";
    str1 = formatter.format(record);

    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    ASSERT_EQ(str2, "[    0.014] T#0  (Worker) [Info    ] [::TestBody:383]  Test");

    // #3
    record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(14);
    record.module = "Module";
    str1 = formatter.format(record);

    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    ASSERT_EQ(str2, "[    0.014] T#0  [Info    ] [Module               ] [::TestBody:393]  Test");

    // #4
    record = _ALOG_RECORD(ALog::Severity::Info) << "Test";
    record.startTp = record.steadyTp - std::chrono::milliseconds(13 * 1000 + 14);
    record.threadTitle = "Worker";
    record.module = "Module";
    str1 = formatter.format(record);

    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    ASSERT_EQ(str2, "[   13.014] T#0  (Worker) [Info    ] [Module               ] [::TestBody:403]  Test");

    // #5
    record.flagsOn(ALog::Record::Flags::Abort);
    str1 = formatter.format(record);

    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    ASSERT_NE(strstr(str2.data(), "test3_alog.cpp"), nullptr);
}

TEST(ALog, test_defaultFormatterLate)
{
    DEFINE_ALOGGER_MODULE(ALogerTest);
    ASSERT_THROW(LOGD << "1" << THROW, ALog::runtime_error_wide);
    ALog::DefaultLogger logger;
    ALog::Record record[2];
    int recordIdx = 0;
    logger->setSink(std::make_shared<ALog::SinkFunctor2>([&](const ALog::Record& rec){
        record[recordIdx++] = rec;
    }));
    logger.markReady();
    LOGD << "Just trigger" << FLUSH;

    ASSERT_EQ(recordIdx, 2);

    ALog::DefaultFormatter formatter;
    auto str1 = formatter.format(record[0]);
    std::string str2;

    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    ASSERT_NE(strstr(str2.data(), "test3_alog.cpp"), nullptr);

    str1 = formatter.format(record[1]);
    str2.resize(str1.size());
    memcpy(str2.data(), str1.data(), str1.size());
    ASSERT_EQ(strstr(str2.data(), "test3_alog.cpp"), nullptr);
    ASSERT_NE(strstr(str2.data(), "Just trigger"), nullptr);
}

TEST(ALog, test_hex)
{
    uint8_t buffer[] = {1, 2, 3, 4, 5, 6};
    char buffer2[1024];

    auto record = _ALOG_RECORD(ALog::Severity::Info) << BUFFER(buffer, sizeof(buffer));
    sprintf(buffer2, "{Buffer; Size: 6, Ptr = 0x%p, Data = 0x010203040506}", buffer);
    ASSERT_EQ(strcmp(record.getMessage(), buffer2), 0);

    record = _ALOG_RECORD(ALog::Severity::Info) << BUFFER(buffer, 0);
    sprintf(buffer2, "{Buffer; Size: 0, Ptr = 0x%p. No data}", buffer);
    ASSERT_EQ(strcmp(record.getMessage(), buffer2), 0);
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
    auto sink2 = std::make_shared<ALog::SinkFunctor2>([&records](const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->setSink(sink2);
    MARK_ALOGGER_READY;

    LOGMD << SEP(" ")       << "String-1" << 1 << "String-2";
    LOGMD <<                   "String-1" << 1 << "String-2";
    LOGMD << NO_SEPARATORS  << "String-1" << 1 << "String-2";
    LOGMD << SEP(", ")      << "String-1" << 1 << "String-2";

    MainALogger_0->flush();

    ASSERT_EQ(records.size(), 4);
    ASSERT_STREQ(records[0].message.getString(), "String-1 1 String-2");
#ifdef ALOG_ENABLE_DEF_SEPARATORS
    ASSERT_STREQ(records[1].message.getString(), "String-1 1 String-2");
#else
    ASSERT_STREQ(records[1].message.getString(), "String-11String-2");
#endif
    ASSERT_STREQ(records[2].message.getString(), "String-11String-2");
    ASSERT_STREQ(records[3].message.getString(), "String-1, 1, String-2");
}

TEST(ALog, test_separators_advanced)
{
#if 0
    std::vector<ALog::Record> records;

    DEFINE_MAIN_ALOGGER;
    auto sink2 = std::make_shared<ALog::SinkFunctor2>([&records](const ALog::Record& rec){ records.push_back(rec); });
    ALOGGER_DIRECT->setSink(sink2);
    MARK_ALOGGER_READY;

    LOGMD << "Literal string" << "SSS" << "(SSS)";
    LOGMD << L"Literal string (wide)";
    LOGMD << std::string("String");
    LOGMD << std::wstring(L"String (wide)");
    LOGMD << std::pair<int, std::string>(11, "Hi") << "SSS";
    LOGMD << std::pair<std::string, int>("Hi", 11) << "SSS";
    LOGMD << NO_SEPARATOR << std::pair<int, std::string>(11, "Hi") << "SSS-no-sep";
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
    ASSERT_STREQ(records[0].message.getString(), "Literal string SSS(SSS)");
    ASSERT_STREQ(records[1].message.getString(), "Literal string (wide)");
    ASSERT_STREQ(records[2].message.getString(), "\"String\"");
    ASSERT_STREQ(records[3].message.getString(), "\"String (wide)\"");
    ASSERT_STREQ(records[4].message.getString(), "(11, \"Hi\") SSS");
    ASSERT_STREQ(records[5].message.getString(), "(\"Hi\", 11) SSS");
    ASSERT_STREQ(records[6].message.getString(), "(11, \"Hi\")SSS-no-sep");
    ASSERT_STREQ(records[7].message.getString(), "(11, (12, \"Hi\"))");
    ASSERT_STREQ(records[8].message.getString(), "((\"1\", \"2\"), (\"3\", \"4\"))");
    ASSERT_STREQ(records[9].message.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4} SSS");
    ASSERT_STREQ(records[10].message.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4}");
    ASSERT_STREQ(records[11].message.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4}");
    //ASSERT_STREQ(records[12].str.getString(), "{Container; Size: 4; Data = 1, 2, 3, 4}");
    ASSERT_STREQ(records[13].message.getString(), "{Container; Size: 4; Data = (1, \"1\"), (2, \"2\"), (3, \"3\"), (4, \"4\")}");
    //ASSERT_STREQ(records[14].str.getString(), "{Container; Size: 4; Data = (1, \"1\"), (2, \"2\"), (3, \"3\"), (4, \"4\")} SSS");
    ASSERT_STREQ(records[15].message.getString(), "{Container; Size: 3; Data = (1, \"1\"), (2, \"2\"), (3, \"3\")}");
    ASSERT_STREQ(records[16].message.getString(), "{Container; Size: 2; Data = {Container; Size: 3; Data = (1, \"1\"), (2, \"2\"), (3, \"3\")}, {Container; Size: 2; Data = (4, \"4\"), (5, \"5\")}}");
#endif
}
