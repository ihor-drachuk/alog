#include <gtest/gtest.h>

#include <cstdint>
#include <string>
#include <utility>

#include <alog/logger.h>
#include <alog/sinks/console.h>
#include <alog/containers/all.h>

#ifdef ALOG_HAS_QT_LIBRARY
#include <QVector>
#include <QList>
#include <QMap>
#include <QSet>
#include <QPair>
#include <QJsonValue>
#include <QJsonObject>
#include <QJsonArray>
#endif // ALOG_HAS_QT_LIBRARY

TEST(ALog_DataTypes, test_various_types)
{
    DEFINE_MAIN_ALOGGER;
    ALOGGER_DIRECT->pipeline().sinks().set(std::make_shared<ALog::Sinks::Console>());
    MARK_ALOGGER_READY;

    DEFINE_ALOGGER_MODULE(ALogerTest);
    const int* ptrType = (const int*)this;
    const void* ptr = ptrType;

    LOGD;
    LOGD << ptrType;
    LOGD << ptr;
    LOGD << 1;
    LOGD << (uint8_t)1;
    LOGD << ( int8_t)1;
    LOGD << (uint16_t)1;
    LOGD << ( int16_t)1;
    LOGD << (uint32_t)1;
    LOGD << ( int32_t)1;
    LOGD << (uint64_t)1;
    LOGD << ( int64_t)1;
    LOGD << "Literal string" << "SSS" << "(SSS)";
    LOGD << L"Literal string (wide)";
    LOGD << std::string("String");
    LOGD << std::wstring(L"String (wide)");
    LOGD << "Some value:" << std::string("value");
    LOGD << std::pair<int, std::string>(11, "Hi") << "SSS";
    LOGD << std::pair<std::string, int>("Hi", 11) << "SSS";
    LOGD << NO_SEPARATORS << std::pair<int, std::string>(11, "Hi") << "SSS-no-sep";
    LOGD << std::pair<int, std::pair<int, std::string>>(11, {12, "Hi"}) ;

    LOGD << std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string>>({"1", "2"}, {"3", "4"}) << "T";
    LOGD << SSEP(20) << std::pair<std::pair<std::string, std::string>, std::pair<std::string, std::string>>({"1", "2"}, {"3", "4"}) << "T";

    LOGD << std::vector<int>{1, 2, 3, 4} << "SSS";
    LOGD << std::list<int>{1, 2, 3, 4};
    LOGD << std::set<int>{1, 2, 3, 4};
    LOGD << std::unordered_set<int>{1, 2, 3, 4};
    LOGD << std::map<int, std::string>{{1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}};
    LOGD << std::unordered_map<int, std::string>{{1, "1"}, {2, "2"}, {3, "3"}, {4, "4"}} << "SSS";

    LOGD << std::vector<std::pair<int, const char*>>{{1, "1"}, {2, "2"}, {3, "3"}};

    LOGD << std::vector<std::vector<std::pair<int, const char*>>> {
        {{1, "1"}, {2, "2"}, {3, "3"}},
        {{4, "4"}, {5, "5"}},
    };

#ifdef ALOG_HAS_QT_LIBRARY
    LOGD << QVector<int>{1,2,3,4};
    LOGD << QList{1,2,3,4};
    LOGD << QMap<int, const char*>{{1, "1"}, {2, "2"}};
    LOGD << QSet{1,2,3,4};
    LOGD << QPair<int, const char*>{1, "11"};
    LOGD << QJsonValue(1234);
    LOGD << QJsonObject({{"field1", 10}, {"field2", "str"}});
    LOGD << QJsonArray({1, 2, "Test"});
#endif // ALOG_HAS_QT_LIBRARY

    LOGD << BUFFER("Hello", 5);

    LOGD << SEP(" ")        << "String-1" << 1 << "String-2";
    LOGD <<                    "String-1" << 1 << "String-2";
    LOGD << NO_SEPARATORS   << "String-1" << 1 << "String-2";
    LOGD << SEP(", ")       << "String-1" << 1 << "String-2";

    LOGD << "Print now" << FLUSH;
}
