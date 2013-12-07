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
    ASSERT_STREQ(testData.c_str(), result.data());
    ASSERT_EQ(testData.size(), result.size());
}

TEST(ValueFormatters, CanFormatIntegers)
{
    auto const result = format_value(42);
    ASSERT_EQ(2, result.size());
    ASSERT_STREQ("42", result.data());
}

TEST(ValueFormatters, CanFormatBigIntegers)
{
    auto const result = format_value(std::numeric_limits<int>::min());
    ASSERT_EQ(11, result.size());
    ASSERT_STREQ("-2147483648", result.data());
}

TEST(WriteFormat, SanityConcat)
{
    string_sink sink;
    write(sink, "Hello", " ", "world!");
    ASSERT_EQ(static_cast<std::string>("Hello world!"), sink.get());
}