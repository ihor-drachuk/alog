#include <gtest/gtest.h>
#include <string>
#include "alog/tools.h"

TEST(ALog_LongSSO, test0)
{
    for (int i = 0; i < 100; i++) {
        ALog::I::LongSSO<> longSSO;
        (void)longSSO;
    }

    ALog::Buffer cache;

    for (int i = 0; i < 100; i++) {
        ALog::I::LongSSO<> longSSO(cache);
        (void)longSSO;
    }
}

TEST(ALog_LongSSO, basic)
{
    ALog::I::LongSSO<> longSSO;
    ASSERT_NE(longSSO.getString(), nullptr);
    ASSERT_EQ(*longSSO.getString(), 0);
    ASSERT_EQ(longSSO.getStringLen(), 0);
    ASSERT_TRUE(longSSO.isShortString());

    auto targetLen = longSSO.getSsoLimit() * 6 / 10;
    std::string someText(targetLen, 'a');

    longSSO.appendString(someText.data(), someText.size());
    ASSERT_EQ(longSSO.getStringLen(), targetLen);
    ASSERT_EQ(memcmp(someText.data(), longSSO.getString(), someText.size()), 0);
    ASSERT_EQ(*(longSSO.getString() + targetLen), 0);
    ASSERT_TRUE(longSSO.isShortString());

    std::string someText2 = someText;
    someText = std::string(targetLen, 'b');
    someText2 += someText;
    auto targetLen2 = targetLen * 2;

    longSSO.appendString(someText.data(), someText.size());
    //auto offset = longSSO.appendString(someText.data(), someText.size());
    //ASSERT_EQ((uint64_t)offset - (uint64_t)longSSO.getString(), targetLen);
    //ASSERT_STREQ(offset, someText.c_str());
    ASSERT_FALSE(longSSO.isShortString());
    ASSERT_EQ(longSSO.getStringLen(), targetLen2);
    ASSERT_EQ(memcmp(someText2.data(), longSSO.getString(), someText2.size()), 0);
    ASSERT_EQ(*(longSSO.getString() + targetLen2), 0);
}

TEST(ALog_LongSSO, formatted)
{
    ALog::I::LongSSO<> sso;

    const char* const model = "Test 11     7";
    const std::string longText(sso.getSsoLimit() * 3 / 2, 'a');
    const std::string model2 = std::string(model) + longText + std::string(model) + std::string(model);

    ALog::Buffer modelBuffer1(strlen(model) + 1);
    ALog::Buffer modelBuffer2(model2.size() + 1);
    memcpy(modelBuffer1.data(), model, strlen(model) + 1);
    memcpy(modelBuffer2.data(), model2.c_str(), model2.size() + 1);

    sso.appendFmtString("%s %d %5d", "Test", 11, 7);
    ASSERT_NE(sso.getString(), nullptr);
    ASSERT_EQ(sso.getStringLen(), strlen(model));
    ASSERT_EQ(memcmp(sso.getString(), model, strlen(model)), 0);
    ASSERT_EQ(*(sso.getString() + sso.getStringLen()), 0);

    sso.appendString(longText.c_str(), longText.size());
    sso.appendFmtString("%s %d %5d", "Test", 11, 7);
    sso.appendFmtString("%s %d %5d", "Test", 11, 7);

    ASSERT_NE(sso.getString(), nullptr);
    ASSERT_EQ(sso.getStringLen(), model2.size());
    ASSERT_EQ(memcmp(sso.getString(), model2.data(), model2.size()), 0);
    ASSERT_EQ(*(sso.getString() + sso.getStringLen()), 0);
}

TEST(ALog_LongSSO, copy)
{
    const char* const model = "Some text";

    {
        ALog::I::LongSSO<> sso, sso2;
        sso.appendStringAL(model);
        ASSERT_EQ(sso.getStringLen(), strlen(model));
        ASSERT_STREQ(sso.getString(), model);
        ASSERT_TRUE(sso.isShortString());

        sso2.appendString("111");
        sso2 = sso;
        ASSERT_EQ(sso2.getStringLen(), sso.getStringLen());
        ASSERT_STREQ(sso2.getString(), model);
        ASSERT_TRUE(sso2.isShortString());

        ASSERT_STREQ(sso.getString(), model);
    }

    {
        ALog::I::LongSSO<5> sso, sso2;
        sso.appendStringAL(model);
        ASSERT_EQ(sso.getStringLen(), strlen(model));
        ASSERT_STREQ(sso.getString(), model);
        ASSERT_FALSE(sso.isShortString());

        sso2.appendString("111");
        sso2 = sso;
        ASSERT_EQ(sso.getStringLen(), sso2.getStringLen());
        ASSERT_STREQ(sso2.getString(), model);
        ASSERT_FALSE(sso2.isShortString());

        ASSERT_STREQ(sso.getString(), model);
    }
}

TEST(ALog_LongSSO, move)
{
    const char* const model = "Some text";

    {
        ALog::I::LongSSO<> sso, sso2;
        sso.appendStringAL(model);
        ASSERT_EQ(sso.getStringLen(), strlen(model));
        ASSERT_STREQ(sso.getString(), model);
        ASSERT_TRUE(sso.isShortString());

        sso2.appendString("111");
        sso2 = std::move(sso);
        ASSERT_EQ(sso2.getStringLen(), strlen(model));
        ASSERT_STREQ(sso2.getString(), model);
        ASSERT_TRUE(sso2.isShortString());
    }

    {
        ALog::I::LongSSO<5> sso, sso2;
        sso.appendStringAL(model);
        ASSERT_EQ(sso.getStringLen(), strlen(model));
        ASSERT_STREQ(sso.getString(), model);
        ASSERT_FALSE(sso.isShortString());

        sso2.appendString("111");
        sso2 = std::move(sso);
        ASSERT_EQ(sso2.getStringLen(), strlen(model));
        ASSERT_STREQ(sso2.getString(), model);
        ASSERT_FALSE(sso2.isShortString());
    }
}

TEST(ALog_LongSSO, string_constructor)
{
    ALog::I::LongSSO<> sso("1");
    ASSERT_EQ(sso.getStringLen(), 1);
    ASSERT_STREQ(sso.getString(), "1");
}
