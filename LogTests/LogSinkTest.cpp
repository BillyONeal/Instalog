// Copyright © 2012-2013 Billy O'Neal III
// This is under the 2 clause BSD license.
// See the included LICENSE.TXT file for more details.

#include "pch.hpp"
#include "gtest/gtest.h"
#include "../LogCommon/LogSink.hpp"

using namespace Instalog;

TEST(StringSink, WriteToStringSink)
{
    string_sink sink;
    sink.append("hello", 5);
    ASSERT_EQ("hello", sink.get());
}

TEST(StringSink, WriteIsNotNullTerminated)
{
    string_sink sink;
    std::string testData("hello");
    testData.push_back('\0');
    testData.append("world");
    sink.append(testData.c_str(), testData.size());
    ASSERT_EQ(testData, sink.get());
}

TEST(ValueFormatters, CanFormatStdString)
{
    std::string testData("example");
    auto const result = format_value(testData);
    ASSERT_EQ(testData.size(), result.size());
    std::string resultStr(result.data(), result.size());
    ASSERT_STREQ(testData.c_str(), resultStr.c_str());
}

TEST(ValueFormatters, CanFormatIntegers)
{
    auto const result = format_value(42);
    ASSERT_EQ(2, result.size());
    std::string resultStr(result.data(), result.size());
    ASSERT_STREQ("42", resultStr.c_str());
}

TEST(ValueFormatters, CanFormatBigIntegers)
{
    auto const result = format_value(std::numeric_limits<int>::min());
    ASSERT_EQ(11, result.size());
    std::string resultStr(result.data(), result.size());
    ASSERT_STREQ("-2147483648", resultStr.c_str());
}

TEST(WriteFormat, SanityConcat)
{
    string_sink sink;
    write(sink, "Hello", " ", "world!");
    ASSERT_EQ(static_cast<std::string>("Hello world!"), sink.get());
}

TEST(WriteFormat, Integer)
{
    std::string sink;
    write(sink, "Value is: ", 1729);
    ASSERT_STREQ("Value is: 1729", sink.c_str());
}

TEST(WriteFormat, WithNewline)
{
    std::string sink;
    writeln(sink, "What follows is a newline:");
    write(sink, sink.size(), " bytes written so far");
#ifdef BOOST_WINDOWS
    char const* expected = "What follows is a newline:\r\n28 bytes written so far";
#else
    char const* expected = "What follows is a newline:\n27 bytes written so far";
#endif
    ASSERT_STREQ(expected, sink.c_str());
}

TEST(WriteFormat, AllIntegralTypes)
{
    short s = -1729;
    int i = -1730;
    long l = -1731;
    long long ll = -1732;
    std::string sink;
    write(sink, "Short: ", s, " Int: ", i, " Long: ", l, " Long Long: ", ll);
    ASSERT_STREQ("Short: -1729 Int: -1730 Long: -1731 Long Long: -1732", sink.c_str());
}

TEST(WriteFormat, AllUnsignedIntegralTypes)
{
    unsigned short s = 1729;
    unsigned int i = 1730;
    unsigned long l = 1731;
    unsigned long long ll = 1732;
    std::string sink;
    write(sink, "Short: ", s, " Int: ", i, " Long: ", l, " Long Long: ", ll);
    ASSERT_STREQ("Short: 1729 Int: 1730 Long: 1731 Long Long: 1732", sink.c_str());
}

TEST(WriteFormat, FloatingPointTypes)
{
    float f = 3.14f;
    double d = 2.718;
    std::string sink;
    write(sink, "Float: ", f, " Double: ", d);
    ASSERT_STREQ("Float: 3.14 Double: 2.718", sink.c_str());
}
